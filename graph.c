/*
 * graph.c - Functions to manipulate and create control flow and
 *           interference graphs.
 */

#include "graph.h"
#include "absyn.h"
#include "assem.h"
#include "errormsg.h"
#include "frame.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "tree.h"
#include "util.h"
#include <stdio.h>

struct G_graph_ {
  int nodecount;
  G_nodeList mynodes, mylast;
};

struct G_node_ {
  G_graph mygraph;
  int mykey;
  G_nodeList succs;
  G_nodeList preds;
  void* info;
  // modify: add set identification
  int lid;
};

G_graph
G_Graph(void)
{
  G_graph g = (G_graph)checked_malloc(sizeof *g);
  g->nodecount = 0;
  g->mynodes = NULL;
  g->mylast = NULL;
  return g;
}

G_nodeList
G_NodeList(G_node head, G_nodeList tail)
{
  G_nodeList n = (G_nodeList)checked_malloc(sizeof *n);
  n->head = head;
  n->tail = tail;
  n->prev = NULL;
  n->id = 0;
  if (tail) {
    tail->prev = n;
  }
  return n;
}

/* generic creation of G_node */
G_node
G_Node(G_graph g, void* info)
{
  G_node n = (G_node)checked_malloc(sizeof *n);
  G_nodeList p = G_NodeList(n, NULL);
  assert(g);
  n->mygraph = g;
  n->mykey = g->nodecount++;

  if (g->mylast == NULL)
    g->mynodes = g->mylast = p;
  else
    g->mylast = g->mylast->tail = p;

  n->succs = NULL;
  n->preds = NULL;
  n->info = info;
  n->lid = -1;
  return n;
}

G_nodeList
G_nodes(G_graph g)
{
  assert(g);
  return g->mynodes;
}

/* return true if a is in l list */
bool
G_inNodeList(G_node a, G_nodeList l)
{
  G_nodeList p;
  for (p = l; p != NULL; p = p->tail)
    if (p->head == a) return TRUE;
  return FALSE;
}

void
G_addEdge(G_node from, G_node to)
{
  assert(from);
  assert(to);
  assert(from->mygraph == to->mygraph);
  if (G_goesTo(from, to)) return;
  to->preds = G_NodeList(from, to->preds);
  from->succs = G_NodeList(to, from->succs);
}

static G_nodeList delete (G_node a, G_nodeList l)
{
  assert(a && l);
  if (a == l->head)
    return l->tail;
  else
    return G_NodeList(l->head, delete (a, l->tail));
}

void
G_rmEdge(G_node from, G_node to)
{
  assert(from && to);
  to->preds = delete (from, to->preds);
  from->succs = delete (to, from->succs);
}

/**
 * Print a human-readable dump for debugging.
 */
void
G_show(FILE* out, G_nodeList p, void showInfo(void*))
{
  for (; p != NULL; p = p->tail) {
    G_node n = p->head;
    G_nodeList q;
    assert(n);
    if (showInfo) showInfo(n->info);
    fprintf(out, " (%d): ", n->mykey);
    for (q = G_succ(n); q != NULL; q = q->tail)
      fprintf(out, "%d ", q->head->mykey);
    fprintf(out, "\n");
  }
}

G_nodeList
G_succ(G_node n)
{
  assert(n);
  return n->succs;
}

G_nodeList
G_pred(G_node n)
{
  assert(n);
  return n->preds;
}

bool
G_goesTo(G_node from, G_node n)
{
  return G_inNodeList(n, G_succ(from));
}

/* return length of predecessor list for node n */
static int
inDegree(G_node n)
{
  int deg = 0;
  G_nodeList p;
  for (p = G_pred(n); p != NULL; p = p->tail)
    deg++;
  return deg;
}

/* return length of successor list for node n */
static int
outDegree(G_node n)
{
  int deg = 0;
  G_nodeList p;
  for (p = G_succ(n); p != NULL; p = p->tail)
    deg++;
  return deg;
}

int
G_degree(G_node n)
{
  return inDegree(n) + outDegree(n);
}

/* put list b at the back of list a and return the concatenated list */
static G_nodeList
cat(G_nodeList a, G_nodeList b)
{
  if (a == NULL)
    return b;
  else
    return G_NodeList(a->head, cat(a->tail, b));
}

/* create the adjacency list for node n by combining the successor and
 * predecessor lists of node n */
G_nodeList
G_adj(G_node n)
{
  return cat(G_succ(n), G_pred(n));
}

void*
G_nodeInfo(G_node n)
{
  return n->info;
}

/* G_node table functions */

G_table
G_empty(void)
{
  return TAB_empty();
}

void
G_enter(G_table t, G_node node, void* value)
{
  TAB_enter(t, node, value);
}

void*
G_look(G_table t, G_node node)
{
  return TAB_look(t, node);
}

G_nodeList
G_except(G_nodeList n1, G_nodeList n2)
{
  if (!n2) return n1;

  G_nodeList r = NULL;
  for (; n1; n1 = n1->tail) {
    if (!G_inlist_brute(n2, n1->head)) r = G_NodeList(n1->head, r);
  }
  return r;
}

static int setcnt = 1;

G_nodeList
G_WorkList()
{
  G_nodeList wl = G_NodeList(NULL, NULL);
  wl->id = setcnt++;
  return wl;
}

void
G_addworklist(G_nodeList* nl, G_node n)
{
  if (*nl) {
    if (!G_inworklist(*nl, n)) {
      n->lid = (*nl)->id;
      *nl = G_NodeList(n, *nl);
    }
  }
  else {
    assert(0);
    *nl = G_NodeList(n, NULL);
  }
}

// different from G_inNodeList. Check identifier only
bool
G_inworklist(G_nodeList nl, G_node n)
{
  assert(nl); // it's a worklist!
  return nl->head && n->lid == nl->head->lid;
  //  inlist_brute(nl, n);
}

bool
G_inlist_brute(G_nodeList nl, G_node n)
{
  for (; nl; nl = nl->tail) {
    if (nl->head == n) return TRUE;
  }
  return FALSE;
}

static G_nodeList
copylist(G_nodeList n)
{
  G_nodeList tl = NULL, hd = NULL;
  for (; n; n = n->tail) {
    if (!tl)
      hd = tl = G_NodeList(n->head, NULL);
    else
      tl = tl->tail = G_NodeList(n->head, NULL);
  }
  return hd;
}

G_nodeList
G_union(G_nodeList n1, G_nodeList n2)
{
  if (!n1) return n2;

  G_nodeList r = copylist(n1);
  for (; n2; n2 = n2->tail) {
    if (!n1 || !G_inlist_brute(n1, n2->head)) r = G_NodeList(n2->head, r);
  }

  return r;
}

G_bmat
G_Bitmatrix(G_graph g)
{
  U_bmat bm = U_Bmat(g->nodecount);
  G_nodeList l1, l2;
  for (l1 = G_nodes(g); l1; l1 = l1->tail) {
    for (l2 = G_nodes(g); l2; l2 = l2->tail) {
      int n1 = l1->head->mykey;
      int n2 = l2->head->mykey;
      bool isadj = G_goesTo(l1->head, l2->head) || G_goesTo(l2->head, l1->head);
      // if (isadj) printf("%d - %d", n1, n2);
      bmenter(bm, n1, n2, isadj);
      bmenter(bm, n2, n1, isadj);
    }
  }
  return bm;
}

bool
G_bmlook(G_bmat bm, G_node n1, G_node n2)
{
  return bmlook(bm, n1->mykey, n2->mykey);
}

void
G_bmenter(G_bmat bm, G_node n1, G_node n2, bool v)
{
  bmenter(bm, n1->mykey, n2->mykey, v);
}
