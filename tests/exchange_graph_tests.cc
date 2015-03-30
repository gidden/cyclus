#include <gtest/gtest.h>

#include "cyc_limits.h"
#include "error.h"
#include "exchange_graph.h"

using cyclus::Arc;
using cyclus::ExchangeGraph;
using cyclus::Match;
using cyclus::ExchangeNode;
using cyclus::ExchangeNodeGroup;
using cyclus::RequestGroup;
using std::vector;


TEST(ExGraphTests, ExchangeNodeGroups) {
  ExchangeNode::Ptr n(new ExchangeNode());
  ExchangeNodeGroup s;
  s.AddExchangeNode(n);
  EXPECT_EQ(&s, n->group);
}


TEST(ExGraphTests, ReqGroups) {
  double q = 1.5;
  RequestGroup r;
  EXPECT_EQ(0, r.qty());
  r = RequestGroup(q);
  EXPECT_EQ(q, r.qty());
}


TEST(ExGraphTests, AddReqGroup) {
  RequestGroup::Ptr prs(new RequestGroup());
  ExchangeGraph g;
  g.AddRequestGroup(prs);
  EXPECT_EQ(g.request_groups().at(0), prs);
}


TEST(ExGraphTests, AddSuppGroup) {
  ExchangeNodeGroup::Ptr pss(new ExchangeNodeGroup());
  ExchangeGraph g;
  g.AddSupplyGroup(pss);
  EXPECT_EQ(g.supply_groups().at(0), pss);
}


TEST(ExGraphTests, AddArc1) {
  ExchangeGraph g;

  ExchangeNode::Ptr u(new ExchangeNode());
  ExchangeNode::Ptr v(new ExchangeNode());

  Arc a(u, v);

  Arc arr[] = {a};
  vector<Arc> exp (arr, arr + sizeof(arr) / sizeof(arr[0]) );

  g.AddArc(a);
  EXPECT_EQ(exp, g.node_arc_map().at(u));
  EXPECT_EQ(exp, g.node_arc_map().at(v));
}


TEST(ExGraphTests, AddArc2) {
  ExchangeGraph g;

  ExchangeNode::Ptr u(new ExchangeNode());
  ExchangeNode::Ptr v(new ExchangeNode());
  ExchangeNode::Ptr w(new ExchangeNode());
  ExchangeNode::Ptr x(new ExchangeNode());

  Arc a1(u, v);
  Arc a2(u, w);
  Arc a3(x, w);

  Arc arru[] = {a1, a2};
  vector<Arc> expu (arru, arru + sizeof(arru) / sizeof(arru[0]) );

  Arc arrv[] = {a1};
  vector<Arc> expv (arrv, arrv + sizeof(arrv) / sizeof(arrv[0]) );

  Arc arrw[] = {a2, a3};
  vector<Arc> expw (arrw, arrw + sizeof(arrw) / sizeof(arrw[0]) );

  Arc arrx[] = {a3};
  vector<Arc> expx (arrx, arrx + sizeof(arrx) / sizeof(arrx[0]) );

  g.AddArc(a1);
  g.AddArc(a2);
  g.AddArc(a3);

  EXPECT_EQ(expu, g.node_arc_map().at(u));
  EXPECT_EQ(expv, g.node_arc_map().at(v));
  EXPECT_EQ(expw, g.node_arc_map().at(w));
  EXPECT_EQ(expx, g.node_arc_map().at(x));
}


TEST(ExGraphTests, AddMatch) {
  ExchangeGraph g;

  double uval = 1.0;
  double vval = 0.5;
  ExchangeNode::Ptr u(new ExchangeNode());
  ExchangeNode::Ptr v(new ExchangeNode());
  Arc a(u, v);

  u->unit_capacities[a].push_back(uval);
  v->unit_capacities[a].push_back(vval);

  double large = 500;
  ExchangeNodeGroup::Ptr ugroup(new ExchangeNodeGroup());
  ugroup->AddExchangeNode(u);
  double ucap = uval * 500;
  ugroup->AddCapacity(ucap);

  ExchangeNodeGroup::Ptr vgroup(new ExchangeNodeGroup());
  vgroup->AddExchangeNode(v);
  double vcap = vval * 500;
  vgroup->AddCapacity(vcap);

  double qty = large * 0.1;

  Match match(std::make_pair(a, qty));

  g.AddMatch(a, qty);
  ASSERT_EQ(1, g.matches().size());
  EXPECT_EQ(match, g.matches().at(0));
}

TEST(ExGraphTests, Part) {
  ExchangeGraph g;
  ExchangeGraph::Ptr gexp1(new ExchangeGraph());
  ExchangeGraph::Ptr gexp2(new ExchangeGraph());
  ExchangeGraph::Ptr aexp[] = {gexp1, gexp2};
  std::set<ExchangeGraph::Ptr> exp(aexp, aexp+2);
  
  // supply block
  ExchangeNode::Ptr u1(new ExchangeNode());
  ExchangeNodeGroup::Ptr gu1(new ExchangeNodeGroup());
  gu1->AddExchangeNode(u1);
  g.AddSupplyGroup(gu1);
  gexp1->AddSupplyGroup(gu1);
  ExchangeNode::Ptr u2(new ExchangeNode());
  ExchangeNodeGroup::Ptr gu2(new ExchangeNodeGroup());
  gu2->AddExchangeNode(u2);
  g.AddSupplyGroup(gu2);
  gexp2->AddSupplyGroup(gu2);

  // request block
  ExchangeNode::Ptr v1(new ExchangeNode());
  RequestGroup::Ptr gv1(new RequestGroup());
  gv1->AddExchangeNode(v1);
  g.AddRequestGroup(gv1);
  gexp1->AddRequestGroup(gv1);
  RequestGroup::Ptr gv2(new RequestGroup());
  ExchangeNode::Ptr v2(new ExchangeNode());
  gv2->AddExchangeNode(v2);
  g.AddRequestGroup(gv2);
  gexp2->AddRequestGroup(gv2);

  // arc block
  Arc a1(u1, v1);
  g.AddArc(a1);
  gexp1->AddArc(a1);
  Arc a2(u2, v2);
  g.AddArc(a2);
  gexp2->AddArc(a2);
  
  std::vector<ExchangeGraph::Ptr> obs = Partition(g);
  // EXPECT_EQ(2, obs.size());
  // EXPECT_EQ(exp, obs);
}

TEST(ExGraphTests, NoPart) {
  ExchangeGraph g;

  ExchangeNode::Ptr u(new ExchangeNode());
  ExchangeNode::Ptr v(new ExchangeNode());
  ExchangeNode::Ptr w(new ExchangeNode());
  ExchangeNode::Ptr x(new ExchangeNode());
  RequestGroup::Ptr prs(new RequestGroup());
  ExchangeNodeGroup::Ptr pss(new ExchangeNodeGroup());

  Arc a1(u, v);
  Arc a2(u, w);
  Arc a3(x, w);

  g.AddSupplyGroup(pss);
  g.AddRequestGroup(prs);

}
