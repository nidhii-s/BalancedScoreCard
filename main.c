#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>      /* for isdigit */
#include "bsc.h"

/* forward declare freeAll in case bsc.h doesn't (your bsc.c implements it) */
void freeAll(Graph *graph);

int main() {
    Graph g;
    initGraph(&g);
    addPerspectiveIfNotExists(&g, "Financial");
addPerspectiveIfNotExists(&g, "Customer");
addPerspectiveIfNotExists(&g, "Internal");
addPerspectiveIfNotExists(&g, "Learning");
/* default dependencies so the app has some initial working data */




    int choice;
    while (1) {
        printf("\n=== Balanced Scorecard System ===\n");
        printf("1. Add Key Performance Indicator (KPI)\n");
        printf("2. View All KPIs\n");
        printf("3. Generate Scorecard (per-KPI performance)\n");
        printf("4. Show Dependencies\n");
        printf("5. Evaluate Performance (averages + dependency impact + lowest performer)\n");
        printf("6. Add Dependency Between Perspectives\n");
        printf("7. Exit\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input.\n");
            while (getchar() != '\n'); // clear buffer
            continue;
        }
        while (getchar() != '\n'); // consume leftover newline

        switch (choice) {
            case 1:
                /* add KPI (interactive) */
                addKPI(&g);
                break;
            case 2:
                /* display all KPIs */
                displayKPIs(&g);
                break;
            case 3:
                /* generate per-KPI scorecard */
                generateScorecard(&g);
                break;
            case 4:
                /* show graph dependencies */
                showDependencies(&g);
                break;
            case 5:
                /* evaluate performance with dependency analysis */
                evaluatePerformanceWithDependencies(&g);
                break;
            case 6: {
                /* interactive dependency input, then call addDependency(graph, from, to) */
                char a[MAX_NAME_LEN], b[MAX_NAME_LEN];

                printf("\nAdd Dependency: A dependency edge A -> B means 'if A performs poorly, it may negatively impact B'.\n");
                printf("Example: Learning -> Internal means poor Learning may lead to weaker Internal processes.\n\n");
                

                displayPerspectives(&g);
                if (g.numNodes == 0) {
                    printf("No perspectives exist yet. Add a perspective by adding a KPI with a new perspective name first.\n");
                    break;
                }

                printf("Enter source perspective (from). You can type the number shown to pick an existing one, or type a NEW name (letters and spaces only): ");
                if (!fgets(a, sizeof(a), stdin)) break;
                a[strcspn(a, "\r\n")] = '\0';
                if (a[0] == '\0') { printf("Source cannot be empty.\n"); break; }

                printf("Enter destination perspective (to). You can type the number shown to pick an existing one, or type a NEW name (letters and spaces only): ");
                if (!fgets(b, sizeof(b), stdin)) break;
                b[strcspn(b, "\r\n")] = '\0';
                if (b[0] == '\0') { printf("Destination cannot be empty.\n"); break; }

                /* Interpret numeric selection or add new name after validation */
                if (isdigit((unsigned char)a[0])) {
                    int idx = atoi(a);
                    if (idx <= 0 || idx > g.numNodes) { printf("Invalid source selection number.\n"); break; }
                    strcpy(a, g.nodes[idx - 1]);
                } else {
                    /* new name should not contain digits */
                    int bad = 0;
                    for (char *p = a; *p; ++p) if (isdigit((unsigned char)*p)) { bad = 1; break; }
                    if (bad) { printf("Perspective names should not contain digits.\n"); break; }
                    addPerspectiveIfNotExists(&g, a);
                }

                if (isdigit((unsigned char)b[0])) {
                    int idx = atoi(b);
                    if (idx <= 0 || idx > g.numNodes) { printf("Invalid destination selection number.\n"); break; }
                    strcpy(b, g.nodes[idx - 1]);
                } else {
                    int bad = 0;
                    for (char *p = b; *p; ++p) if (isdigit((unsigned char)*p)) { bad = 1; break; }
                    if (bad) { printf("Perspective names should not contain digits.\n"); break; }
                    addPerspectiveIfNotExists(&g, b);
                }

                /* call the core graph function that takes (graph, from, to) */
                addDependency(&g, a, b);
                break;
            }
            case 7:
                printf("Exiting program...\n");
                freeAll(&g);
                exit(0);
            default:
                printf("Invalid choice. Please select between 1–7.\n");
        }
    }

    return 0;
}
