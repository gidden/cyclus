#ifndef CYCLUS_SRC_CAPACITY_CONSTRAINT_H_
#define CYCLUS_SRC_CAPACITY_CONSTRAINT_H_

#include <assert.h>

#include <boost/shared_ptr.hpp>

#include "exchange_graph.h"
#include "exchange_translation_context.h"
#include "capacity_types.h"

namespace cyclus {

/// @class Converter
///
/// @brief a simple interface for converting resource objects to unit capacities
template<class T>
struct Converter {
  typedef boost::shared_ptr< Converter<T> > Ptr;

  /// @brief convert a capacitated quantity for an offer in its exchange context
  /// @param offer the resource being offered
  /// @param a the associated arc for the potential offer
  /// @param ctx the exchange context in which the offer is being made
  ///
  /// @warning it is up to the user to inherit default parameters
  virtual double convert(
      boost::shared_ptr<T> offer,
      Arc const * a = NULL,
      ExchangeTranslationContext<T> const * ctx = NULL) const = 0;

  /// @brief operator== is available for subclassing, see
  /// cyclus::TrivialConverter for an example
  virtual bool operator==(Converter& other) const {
    return false;
  }
  bool operator!=(Converter& other) const {
    return !operator==(other);
  }
};

/// @class TrivialConverter
///
/// @brief The default converter returns the resource's quantity
template<class T>
struct TrivialConverter : public Converter<T> {
  /// @returns the quantity of resource offer
  inline virtual double convert(
      boost::shared_ptr<T> offer,
      Arc const * a = NULL,
      ExchangeTranslationContext<T> const * ctx = NULL) const  {
    return offer->quantity();
  }

  /// @returns true if a dynamic cast succeeds
  virtual bool operator==(Converter<T>& other) const {
    return dynamic_cast<TrivialConverter<T>*>(&other) != NULL;
  }
};

/// @class CapacityConstraint
///
/// @brief A CapacityConstraint provides an ability to determine an agent's
/// constraints on resource allocation given a capacity.
template <class T>
class CapacityConstraint {
 public:
  /// @brief constructor for a constraint with a non-trivial converter
  CapacityConstraint(double capacity, typename Converter<T>::Ptr converter)
      : capacity_(capacity),
        converter_(converter),
        cap_type_(NONE),
        id_(next_id_++) {
    assert(capacity_ > 0);
  }

  /// @brief constructor for a constraint with a trivial converter (i.e., one
  /// that simply returns 1)
  explicit CapacityConstraint(double capacity)
      : capacity_(capacity),
        cap_type_(NONE),
        id_(next_id_++) {
    converter_ = typename Converter<T>::Ptr(new TrivialConverter<T>());
    assert(capacity_ > 0);
  }

  /// @brief constructor for a constraint with a non-trivial converter
  CapacityConstraint(const CapacityConstraint& other)
      : capacity_(other.capacity_),
        converter_(other.converter_),
        cap_type_(other.cap_type_),
        id_(next_id_++) {
    assert(capacity_ > 0);
  }

  /// capacity getters/setters
  // @{
  inline double capacity() const { return capacity_; }
  inline void capacity(double c) { capacity_ = c; }
  // @}
  
  /// type getters/setters
  // @{
  inline cap_t cap_type() const { return cap_type_; }
  inline void cap_type(cap_t t) const { cap_type_ = t; }
  // @}
  
  /// @return the converter
  inline typename Converter<T>::Ptr converter() const {
    return converter_;
  }

  inline double convert(
      boost::shared_ptr<T> offer,
      Arc const * a = NULL,
      ExchangeTranslationContext<T> const * ctx = NULL) const {
    return converter_->convert(offer, a, ctx);
  }

  /// @return a unique id for the constraint
  inline int id() const {
    return id_;
  }

 private:
  double capacity_;
  cap_t cap_type_;
  typename Converter<T>::Ptr converter_;
  int id_;
  static int next_id_;
};

template<class T> int CapacityConstraint<T>::next_id_ = 0;

/// @brief CapacityConstraint-CapacityConstraint equality operator
template<class T>
inline bool operator==(const CapacityConstraint<T>& lhs,
                       const CapacityConstraint<T>& rhs) {
  return  ((lhs.capacity() == rhs.capacity()) &&
           (*lhs.converter() == *rhs.converter()) &&
           (lhs.cap_type() == rhs.cap_type()));
}

/// @brief CapacityConstraint-CapacityConstraint comparison operator, allows
/// usage in ordered containers
template<class T>
inline bool operator<(const CapacityConstraint<T>& lhs,
                      const CapacityConstraint<T>& rhs) {
  return  (lhs.id() < rhs.id());
}

}  // namespace cyclus

#endif  // CYCLUS_SRC_CAPACITY_CONSTRAINT_H_
