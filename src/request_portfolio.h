#ifndef CYCLUS_REQUEST_PORTFOLIO_H_
#define CYCLUS_REQUEST_PORTFOLIO_H_

#include <numeric>
#include <set>
#include <string>
#include <vector>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "capacity_constraint.h"
#include "error.h"
#include "logger.h"
#include "request.h"

namespace cyclus {

class Trader;

/// @brief accumulator sum for request quantities
template<class T>
    inline double Sum(int total, typename Request<T>::Ptr r) {
  return total += r->target()->quantity();
};
/// template<class T>
///     inline double Sum(typename Request<T>::Ptr r1,
///                       typename Request<T>::Ptr r2) {
///   return r1->target()->quantity() + r2->target()->quantity();
/// };

/// @brief a default coefficient converter applies default mass constraint
/// coefficients as conversion coefficients for a constraint
template<class T>
struct DefaultCoeffConverter: public Converter<T> {
  DefaultCoeffConverter(
      const std::map<typename Request<T>::Ptr, double>& coeffs)
      : coeffs(coeffs) { };

  inline virtual double convert(
      boost::shared_ptr<T> offer, 
      Arc const * a,
      ExchangeTranslationContext<T> const * ctx) const {
    return offer->quantity() * coeffs.at(ctx->node_to_request.at(a->unode()));
  }

  /// virtual bool operator==(Converter<T>& other) const {
  ///   return dynamic_cast<DefaultCoeffConverter<T>*>(&other) != NULL &&
  ///   coeffs == other.coeffs;
  /// }
  
  std::map<typename Request<T>::Ptr, double> coeffs;
};


/// @class RequestPortfolio
/// 
/// @brief A RequestPortfolio is a group of (possibly constrainted) requests for
/// resources
///
/// The portfolio contains a grouping of resource requests that may be mutually
/// met by suppliers. These requests may share a common set of
/// constraints. Take, for instance, a facility that needs fuel, of which there are
/// two commodity types, fuelA and fuelB. If some combination of the two suffice
/// the facility's needs, then requests for both would be added to the portfolio
/// along with a capacity constraint.
///
/// An option exists to add a default mass based constraint that incorporates
/// multicommodity requests, but it must be called manually once all requests
/// have been added, e.g.,
/// @begincode
/// 
/// RequestPortfolio<SomeResource>::Ptr rp(new RequestPortfolio<SomeResource>());
/// // add some requests
/// // declare some of them as multicommodity requsts (i.e., any one will
/// // satisfy this demand).
/// rp->AddDefaultConstraint();
/// 
/// @endcode
template<class T>
class RequestPortfolio :
public boost::enable_shared_from_this< RequestPortfolio<T> > {
 public:
  typedef boost::shared_ptr< RequestPortfolio<T> > Ptr;

  /// @brief default constructor
  RequestPortfolio() : requester_(NULL), qty_(-1) {};

  /// @brief add a request to the portfolio
  /// @param target the target resource associated with this request
  /// @param requester the requester
  /// @param commodity the commodity associated with this request
  /// @param preference the preference associated with this request (relative to
  /// others in the portfolio)
  /// @param exclusive a flag denoting that this request must be met exclusively,
  /// i.e., in its entirety by a single offer
  /// @throws KeyError if a request is added from a different requester than the
  /// original or if the request quantity is different than the original
  typename Request<T>::Ptr AddRequest(boost::shared_ptr<T> target,
                                      Trader* requester,
                                      std::string commodity = "",
                                      double preference = 0,
                                      bool exclusive = false) {
    typename Request<T>::Ptr r =
        Request<T>::Create(target, requester, this->shared_from_this(),
                           commodity, preference, exclusive);
    VerifyRequester_(r);
    VerifyQty_(r);
    requests_.push_back(r);
    default_constr_coeffs_[r] = 1;
    return r;
  };

  /// @brief adds a collection of requests registered with this portfolio as
  /// multicommodity requests
  /// @param rs the collection of requests to add
  inline void AddMutualReqs(
      const std::vector<typename Request<T>::Ptr>& rs) {
    double norm_const =
        std::accumulate(rs.begin(), rs.end(), 0, Sum<T>) / rs.size();
    for (int i = 0; i < rs.size(); i++) {
      typename Request<T>::Ptr r = rs[i];
      default_constr_coeffs_[r] = r->target()->quantity() / norm_const;
    }
  }
  
  /// @brief add a capacity constraint associated with the portfolio, if it
  /// doesn't already exist
  /// @param c the constraint to add
  inline void AddConstraint(const CapacityConstraint<T>& c) {
    constraints_.insert(c);
  };

  /// @brief adds a default mass constraint based on the current requests and
  /// multicommodity requests
  inline void AddDefaultConstraint() { 
    typename Converter<T>::Ptr conv(
        new DefaultCoeffConverter<T>(default_constr_coeffs_));
    CapacityConstraint<T> c(qty_, conv); // @TODO fix qty_ so this is correct
    constraints_.insert(c);
  };
      
  /// @return the agent associated with the portfolio. if no reqeusts have
  /// been added, the requester is NULL.
  inline Trader* requester() const { return requester_; }

  /// @return the request quantity associated with the portfolio.
  ///
  /// @brief, all requests in a portfolio must have the same quantity, which is
  /// checked during AddRequest()
  inline double qty() const { return qty_; }

  /// @return const access to the unconstrained requests
  inline const std::vector<typename Request<T>::Ptr>& requests() const {
    return requests_;
  };
  
  /// @return const access to the request constraints
  inline const std::set< CapacityConstraint<T> >& constraints() const {
    return constraints_;
  };

 private:
  /// @brief copy constructor is private to prevent copying and preserve
  /// explicit single-ownership of requests
  RequestPortfolio(const RequestPortfolio& rhs) {
    requester_ = rhs.requester_;
    requests_ = rhs.requests_;
    constraints_ = rhs.constraints_;
    qty_ = rhs.qty_;
    typename std::vector<typename Request<T>::Ptr>::iterator it;
    for (it = requests_.begin(); it != requests_.end(); ++it) {
      it->get()->set_portfolio(this->shared_from_this());
    }
  };

  /// @brief if the requester has not been determined yet, it is set. otherwise
  /// VerifyRequester() verifies the the request is associated with the portfolio's
  /// requester
  /// @throws KeyError if a request is added from a different requester than the
  /// original
  void VerifyRequester_(const typename Request<T>::Ptr r) {
    if (requester_ == NULL) {
      requester_ = r->requester();
    } else if (requester_ != r->requester()) {
      std::string msg = "Insertion error: requesters do not match.";
      throw KeyError(msg);
    }
  };

  /// @brief if the quantity has not been determined yet, it is set. otherwise
  /// VerifyRequester() verifies the the quantity is the same as all others in
  /// the portfolio
  /// @throws KeyError if a quantity is different than the original
  void VerifyQty_(const typename Request<T>::Ptr r) {
    double qty = r->target()->quantity();
    if (qty_ == -1) {
      qty_ = qty;
    } else if (qty_ != qty) {
      std::string msg = "Insertion error: request quantity do not match.";
      throw KeyError(msg);
    }
  };

  /// requests_ is a vector because many requests may be identical, i.e., a set
  /// is not appropriate
  std::vector<typename Request<T>::Ptr> requests_;

  /// coefficients for the default mass constraint for known resources
  std::map<typename Request<T>::Ptr, double> default_constr_coeffs_;

  /// constraints_ is a set because constraints are assumed to be unique
  std::set< CapacityConstraint<T> > constraints_;

  double qty_;
  Trader* requester_;
};

} // namespace cyclus

#endif
