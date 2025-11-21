#include "bsc.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* ---------- Helpers ---------- */

/* safe lowercase copy into out (caller ensures out length) */
static void toLowerCopy(char *out, const char *in, int outlen) {
    if (!in || !out) return;
    int i;
    for (i = 0; i < outlen - 1 && in[i]; ++i) out[i] = (char)tolower((unsigned char)in[i]);
    out[i] = '\0';
}

/* case-insensitive compare */
static int strcmp_ci(const char *a, const char *b) {
    if (!a || !b) return (a==b)?0:(a?1:-1);
    while (*a && *b) {
        char ca = (char)tolower((unsigned char)*a);
        char cb = (char)tolower((unsigned char)*b);
        if (ca != cb) return (ca < cb) ? -1 : 1;
        ++a; ++b;
    }
    if (*a) return 1;
    if (*b) return -1;
    return 0;
}

/* check if a string is all digits (ignoring leading/trailing spaces) */
static int isAllDigits(const char *s) {
    if (!s) return 0;
    while (isspace((unsigned char)*s)) ++s;
    if (*s == '\0') return 0;
    while (*s) {
        if (!isdigit((unsigned char)*s) && !isspace((unsigned char)*s)) return 0;
        ++s;
    }
    return 1;
}

/* check if string contains any digit */
static int containsDigit(const char *s) {
    if (!s) return 0;
    while (*s) { if (isdigit((unsigned char)*s)) return 1; ++s; }
    return 0;
}

/* ---------- BST functions (PersNode) ---------- */

/* create a new BST perspective node */
static PersNode *createPersNode(const char *name) {
    PersNode *p = (PersNode*)malloc(sizeof(PersNode));
    if (!p) { perror("malloc"); exit(EXIT_FAILURE); }
    strncpy(p->name, name, MAX_NAME_LEN-1);
    p->name[MAX_NAME_LEN-1] = '\0';
    p->kpiList = NULL;
    p->left = p->right = NULL;
    return p;
}

/* insert into BST using case-sensitive order for structure determinism,
   but equality checks elsewhere should be case-insensitive */
static PersNode *bst_insert(PersNode *root, const char *name) {
    if (!root) return createPersNode(name);
    int cmp = strcmp(name, root->name);
    if (cmp < 0) root->left = bst_insert(root->left, name);
    else if (cmp > 0) root->right = bst_insert(root->right, name);
    return root;
}

/* search BST by name, case-insensitive; returns pointer or NULL */
static PersNode *bst_search_ci(PersNode *root, const char *name) {
    if (!root) return NULL;
    if (strcmp_ci(root->name, name) == 0) return root;
    PersNode *L = bst_search_ci(root->left, name);
    if (L) return L;
    return bst_search_ci(root->right, name);
}

/* inorder traversal with callback (callback receives PersNode* and user pointer) */
typedef void (*PersCallback)(PersNode *, void *);
static void bst_inorder(PersNode *root, PersCallback cb, void *ud) {
    if (!root) return;
    bst_inorder(root->left, cb, ud);
    cb(root, ud);
    bst_inorder(root->right, cb, ud);
}

/* free KPIs for one node */
static void freeKPIList(KPI *k) {
    while (k) {
        KPI *nx = k->next;
        free(k);
        k = nx;
    }
}

/* free BST (nodes + their KPI lists) */
static void bst_free_all(PersNode *root) {
    if (!root) return;
    bst_free_all(root->left);
    bst_free_all(root->right);
    freeKPIList(root->kpiList);
    free(root);
}

/* ---------- Graph + mapping functions ---------- */

/* initialize Graph */
void initGraph(Graph *graph) {
    if (!graph) return;
    graph->numNodes = 0;
    graph->bstRoot = NULL;
    for (int i = 0; i < MAX_PERSPECTIVES; ++i) {
        graph->nodes[i][0] = '\0';
        for (int j = 0; j < MAX_PERSPECTIVES; ++j) graph->adj[i][j] = 0;
    }
}

/* return index in graph->nodes for a name (case-insensitive), or -1 */
int findPerspective(const Graph *graph, const char *name) {
    if (!graph || !name) return -1;
    char tmp[MAX_NAME_LEN];
    toLowerCopy(tmp, name, sizeof(tmp));
    for (int i = 0; i < graph->numNodes; ++i) {
        char cur[MAX_NAME_LEN];
        toLowerCopy(cur, graph->nodes[i], sizeof(cur));
        if (strcmp(tmp, cur) == 0) return i;
    }
    return -1;
}

/* Add perspective into mapping and BST if not present */
void addPerspectiveIfNotExists(Graph *graph, const char *name) {
    if (!graph || !name || name[0] == '\0') return;
    if (findPerspective(graph, name) != -1) return; /* already mapped */

    if (graph->numNodes >= MAX_PERSPECTIVES) {
        printf("Cannot add perspective — limit reached (%d).\n", MAX_PERSPECTIVES);
        return;
    }

    /* add to mapping list (preserve original case as given) */
    strncpy(graph->nodes[graph->numNodes], name, MAX_NAME_LEN-1);
    graph->nodes[graph->numNodes][MAX_NAME_LEN-1] = '\0';
    graph->numNodes++;

    /* insert into BST as well */
    graph->bstRoot = bst_insert(graph->bstRoot, name);
}

/* add directed edge from->to */
void addDependency(Graph *graph, const char *from, const char *to) {
    if (!graph || !from || !to) return;
    /* ensure both exist in mapping (and BST) */
    addPerspectiveIfNotExists(graph, from);
    addPerspectiveIfNotExists(graph, to);
    int fi = findPerspective(graph, from);
    int ti = findPerspective(graph, to);
    if (fi == -1 || ti == -1) {
        printf("One or both perspectives not found (unexpected).\n");
        return;
    }
    if (!graph->adj[fi][ti]) {
        graph->adj[fi][ti] = 1;
        printf("Added dependency: %s -> %s\n", graph->nodes[fi], graph->nodes[ti]);
    } else {
        printf("Dependency already exists: %s -> %s\n", graph->nodes[fi], graph->nodes[ti]);
    }
}

/* Display adjacency list of dependencies */
void showDependencies(const Graph *graph) {
    if (!graph) return;
    printf("\n--- Perspective Dependencies ---\n");
    if (graph->numNodes == 0) { printf("  (no perspectives defined)\n"); return; }
    for (int i = 0; i < graph->numNodes; ++i) {
        printf("%s -> ", graph->nodes[i]);
        int found = 0;
        for (int j = 0; j < graph->numNodes; ++j) {
            if (graph->adj[i][j]) {
                if (found) printf(", ");
                printf("%s", graph->nodes[j]);
                found = 1;
            }
        }
        if (!found) printf("None");
        printf("\n");
    }
}

/* ---------- KPI operations (now per-perspective inside BST) ---------- */

/* interactive addKPI: prompts user for perspective and KPI; auto-creates perspective
   only when a non-numeric name is entered (numeric input selects an existing index). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bsc.h"

void addKPI(Graph *graph) {
    if (!graph) return;

    printf("\n=== Add New Key Performance Indicator (KPI) ===\n");

    /* --- Step 1: Display existing perspectives --- */
    printf("\nExisting Perspectives (count = %d):\n", graph->numNodes);
    if (graph->numNodes == 0) {
        printf("  (no perspectives yet — adding a new one will create it)\n");
    } else {
        for (int i = 0; i < graph->numNodes; ++i) {
            printf("  %d. %s\n", i + 1, graph->nodes[i]);
        }
    }

    /* --- Step 2: Ask for perspective name --- */
    char perspective[MAX_NAME_LEN];
    printf("\nEnter Perspective name (choose by number OR type perspective name): ");
    if (!fgets(perspective, sizeof(perspective), stdin)) return;
    perspective[strcspn(perspective, "\r\n")] = '\0';

    if (perspective[0] == '\0') {
        printf("Perspective name cannot be empty.\n");
        return;
    }

    /* --- Step 3: Handle numeric input properly --- */
    if (isdigit((unsigned char)perspective[0])) {
        int idx = atoi(perspective);
        if (idx <= 0 || idx > graph->numNodes) {
            printf("Invalid number. Please choose a valid perspective number.\n");
            return;
        }
        strcpy(perspective, graph->nodes[idx - 1]);
    } else {
        /* Reject digits in perspective names */
        for (char *p = perspective; *p; ++p) {
            if (isdigit((unsigned char)*p)) {
                printf("Perspective names cannot contain digits.\n");
                return;
            }
        }
        addPerspectiveIfNotExists(graph, perspective);
    }

    /* --- Step 4: Input KPI details --- */
    char kpiName[MAX_NAME_LEN];
    float target, achieved;

    printf("Enter full name of the Key Performance Indicator: ");
    if (!fgets(kpiName, sizeof(kpiName), stdin)) return;
    kpiName[strcspn(kpiName, "\r\n")] = '\0';
    if (kpiName[0] == '\0') {
        printf("KPI name cannot be empty.\n");
        return;
    }

    printf("Enter Target value (1-100): ");

    if (scanf("%f", &target) != 1 || target < 1.0f || target > 100.0f) {
        printf("Invalid target. Must be between 1 and 100.\n");
        while (getchar() != '\n');
        return;
    }

    printf("Enter Achieved value (can exceed target if performance is high): ");
    if (scanf("%f", &achieved) != 1 || achieved < 0.0f) {
        printf("Invalid achieved value.\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n'); // clear buffer

    /* --- Step 5: Add KPI node --- */
    PersNode *pnode = graph->bstRoot;
    while (pnode) {
        int cmp = strcasecmp(perspective, pnode->name);
        if (cmp == 0) break;
        else if (cmp < 0) pnode = pnode->left;
        else pnode = pnode->right;
    }

    if (!pnode) {
        printf("Unexpected error: perspective not found.\n");
        return;
    }

    KPI *newKPI = (KPI *)malloc(sizeof(KPI));
    strcpy(newKPI->name, kpiName);
    newKPI->target = target;
    newKPI->achieved = achieved;
    newKPI->next = pnode->kpiList;
    pnode->kpiList = newKPI;

    printf("\n Key Performance Indicator added successfully under '%s'.\n", perspective);
}


/* display all KPIs by traversing BST inorder and printing each node's KPIs */
static void printNodeKPIs(PersNode *node, void *ud) {
    (void)ud;
    printf("\nPerspective: %s\n", node->name);
    if (!node->kpiList) {
        printf("  (No Key Performance Indicators yet)\n");
        return;
    }
    KPI *t = node->kpiList;
    while (t) {
        float perf = 0.0f;
        if (t->target != 0.0f) perf = (t->achieved / t->target) * 100.0f;

        /* choose colour based on perf */
        const char *col = ANSI_RESET;
        if (perf > 100.0f) col = ANSI_BLUE;          /* More than 100% */
        else if (perf >= 80.0f) col = ANSI_GREEN;    /* Meeting expectation */
        else if (perf >= 20.0f) col = ANSI_YELLOW;   /* Amber (20% - 79.99%) */
        else col = ANSI_RED;                         /* < 20% */

        printf("  - %s | Target: %.2f | Achieved: %.2f | Performance: %s%.2f%%%s\n",
               t->name, t->target, t->achieved, col, perf, ANSI_RESET);
        t = t->next;
    }
}


void displayKPIs(const Graph *graph) {
    if (!graph) return;
    if (!graph->bstRoot) {
        printf("No perspectives / Key Performance Indicators defined yet.\n");
        return;
    }
    bst_inorder(graph->bstRoot, (PersCallback)printNodeKPIs, NULL);
}

/* Generate scorecard: simple list of KPI performances by visiting BST */
void generateScorecard(const Graph *graph) {
    if (!graph) return;
    if (!graph->bstRoot) { printf("No data to generate scorecard.\n"); return; }

    printf("\n=== Scorecard (per Key Performance Indicator performance) ===\n");
    /* reuse printNodeKPIs which prints KPIs with performance */
    bst_inorder(graph->bstRoot, (PersCallback)printNodeKPIs, NULL);
}

/* ---------- NEW: computeScores helper ---------- */
/*
   computeScores fills the provided arrays (totalPerf[] and count[]) by traversing
   the BST. It is used by evaluatePerformanceWithDependencies.
*/
typedef struct {
    const Graph *graph;
    float *totalPerf;
    int *count;
} ComputeCtx;

static void computeScoresNode(PersNode *node, void *ud) {
    ComputeCtx *ctx = (ComputeCtx*)ud;
    if (!node || !ctx) return;
    int idx = findPerspective(ctx->graph, node->name);
    if (idx == -1) return;
    float sum = 0.0f;
    int c = 0;
    KPI *k = node->kpiList;
    while (k) {
        if (k->target != 0.0f) {
            sum += (k->achieved / k->target) * 100.0f;
            c++;
        }
        k = k->next;
    }
    if (c > 0) {
        ctx->totalPerf[idx] = sum;
        ctx->count[idx] = c;
    } else {
        ctx->totalPerf[idx] = 0.0f;
        ctx->count[idx] = 0;
    }
}

static void computeScores(const Graph *graph, float totalPerf[], int count[]) {
    if (!graph) return;
    ComputeCtx ctx;
    ctx.graph = graph;
    ctx.totalPerf = totalPerf;
    ctx.count = count;
    /* initialize arrays */
    for (int i = 0; i < MAX_PERSPECTIVES; ++i) {
        totalPerf[i] = 0.0f;
        count[i] = 0;
    }
    if (graph->bstRoot)
        bst_inorder(graph->bstRoot, computeScoresNode, &ctx);
}

void evaluatePerformanceWithDependencies(const Graph *graph) {
    if (!graph) return;
    if (!graph->bstRoot || graph->numNodes == 0) {
        printf("No data to evaluate.\n");
        return;
    }

    /* arrays to be filled by computeScores */
    float totalPerf[MAX_PERSPECTIVES];
    int count[MAX_PERSPECTIVES];

    computeScores(graph, totalPerf, count);

    /* print averages */
    printf("\n--- Perspective Averages ---\n");
    float avg[MAX_PERSPECTIVES];
    for (int i = 0; i < MAX_PERSPECTIVES; ++i) avg[i] = 0.0f;

    int present = 0;
    float overallSum = 0.0f;
    for (int i = 0; i < graph->numNodes; ++i) {
        if (count[i] > 0) {
            avg[i] = totalPerf[i] / (float)count[i];

            const char *col = ANSI_RESET;
            if (avg[i] > 100.0f) col = ANSI_BLUE;      /* More than 100% */
            else if (avg[i] >= 80.0f) col = ANSI_GREEN;/* Meeting expectation */
            else if (avg[i] >= 20.0f) col = ANSI_YELLOW;/* Amber (20% - <80%) */
            else col = ANSI_RED;                       /* <20% */

            printf("%s: %s%.2f%%%s\n", graph->nodes[i], col, avg[i], ANSI_RESET);

            overallSum += avg[i];
            present++;
        } else {
            avg[i] = 0.0f;
            printf("%s: (No KPI data)\n", graph->nodes[i]);
        }
    }

    float overall = (present > 0) ? (overallSum / (float)present) : 0.0f;

    /* dependency impact analysis */
    printf("\n--- Dependency Impact Analysis ---\n");
    int anyImpact = 0;
    for (int i = 0; i < graph->numNodes; ++i) {
        for (int j = 0; j < graph->numNodes; ++j) {
            if (graph->adj[i][j] && avg[i] > 0.0f && avg[i] < 80.0f) {
                const char *col = (avg[i] < 20.0f) ? ANSI_RED : ANSI_YELLOW;
                printf("%sLow performance in %s (%.2f%%) may affect %s.%s\n",
                       col, graph->nodes[i], avg[i], graph->nodes[j], ANSI_RESET);
                anyImpact = 1;
            }
        }
    }
    if (!anyImpact) {
        printf("No dependency impacts detected based on current averages (threshold: < 80%%).\n");
    }

    /* find lowest performer (among those with KPI data) */
    float min = 1e9f;
    int minIdx = -1;
    for (int i = 0; i < graph->numNodes; ++i) {
        if (count[i] > 0 && avg[i] < min) {
            min = avg[i];
            minIdx = i;
        }
    }

    printf("\nOverall Performance: %.2f%%\n", overall);
    if (minIdx != -1) {
        const char *col = ANSI_RESET;
        if (avg[minIdx] > 100.0f) col = ANSI_BLUE;
        else if (avg[minIdx] >= 80.0f) col = ANSI_GREEN;
        else if (avg[minIdx] >= 20.0f) col = ANSI_YELLOW;
        else col = ANSI_RED;
        printf("Lowest Performing Perspective: %s (%s%.2f%%%s)\n",
               graph->nodes[minIdx], col, avg[minIdx], ANSI_RESET);
    } else {
        printf("No perspective had KPI data to determine lowest performer.\n");
    }
}

/* display numbered list of perspectives using mapping order */
void displayPerspectives(const Graph *graph)
{
    if (!graph) { printf("(No graph available)\n"); return; }
    printf("\nExisting Perspectives (count = %d):\n", graph->numNodes);
    if (graph->numNodes == 0) { printf("  (no perspectives defined)\n"); return; }
    for (int i = 0; i < graph->numNodes; ++i) {
        printf("  %d. %s\n", i + 1, graph->nodes[i]);
    }
}

/* free all memory: free BST (which frees KPI lists) and reset mapping */
void freeAll(Graph *graph) {
    if (!graph) return;
    bst_free_all(graph->bstRoot);
    graph->bstRoot = NULL;
    graph->numNodes = 0;
    for (int i = 0; i < MAX_PERSPECTIVES; ++i) {
        graph->nodes[i][0] = '\0';
        for (int j = 0; j < MAX_PERSPECTIVES; ++j) graph->adj[i][j] = 0;
    }
}
