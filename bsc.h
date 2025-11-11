#ifndef BSCC_H
#define BSCC_H

#include <stdio.h>

#define MAX_PERSPECTIVES 10
#define MAX_NAME_LEN 50

/* KPI linked list node (stored per-perspective inside BST nodes) */
typedef struct KPI {
    char name[MAX_NAME_LEN];
    float target;
    float achieved;
    struct KPI *next;
} KPI;

/* BST node: a perspective with its own KPI linked list and BST children */
typedef struct PersNode {
    char name[MAX_NAME_LEN];
    KPI *kpiList;
    struct PersNode *left;
    struct PersNode *right;
} PersNode;

/* Graph adjacency mapping for dependencies
   - nodes[] stores names in insertion order and is used for adjacency indices
   - numNodes is the number of mapped perspective names (<= MAX_PERSPECTIVES)
   - bstRoot points to the BST root containing PersNode nodes (same names) */
typedef struct Graph {
    char nodes[MAX_PERSPECTIVES][MAX_NAME_LEN];
    int adj[MAX_PERSPECTIVES][MAX_PERSPECTIVES];
    int numNodes;
    PersNode *bstRoot;
} Graph;

/* Initialize graph structure and BST root */
void initGraph(Graph *graph);

/* Add perspective (case-insensitive) - ensures mapping and BST node exist */
void addPerspectiveIfNotExists(Graph *graph, const char *name);

/* Find the index in graph->nodes[] for a perspective name (case-insensitive).
   Returns -1 if not present. */
int findPerspective(const Graph *graph, const char *name);

/* Add directed dependency edge from -> to */
void addDependency(Graph *graph, const char *from, const char *to);

/* Print adjacency list of dependencies */
void showDependencies(const Graph *graph);

/* Add Key Performance Indicator (interactive) */
void addKPI(Graph *graph);

/* Display all Key Performance Indicators by traversing the BST (inorder) */
void displayKPIs(const Graph *graph);

/* Compute and print KPI performance per KPI (simple list) */
void generateScorecard(const Graph *graph);

/* Aggregate averages per perspective, then report dependency impacts */
void evaluatePerformanceWithDependencies(const Graph *graph);

/* Print existing perspectives (numbered list) */
void displayPerspectives(const Graph *graph);

/* Free all dynamically allocated KPIs and BST nodes */
void freeAll(Graph *graph);

#endif /* BSCC_H */
