#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bsc.h"

int main(void) {
    Graph g;
    initGraph(&g);

    /* Add default perspectives and default dependency chain */
    addPerspectiveIfNotExists(&g, "Financial");
    addPerspectiveIfNotExists(&g, "Customer");
    addPerspectiveIfNotExists(&g, "Internal");
    addPerspectiveIfNotExists(&g, "Learning");

    addDependency(&g, "Learning", "Internal");
    addDependency(&g, "Internal", "Customer");
    addDependency(&g, "Customer", "Financial");

    int choice;
    char a[MAX_NAME_LEN], b[MAX_NAME_LEN];

    while (1) {
        printf("\n====== BALANCED SCORECARD GENERATOR ======\n");
        printf("1. Add KPI\n");
        printf("2. Display All KPIs\n");
        printf("3. Generate Scorecard (compute KPI performance)\n");
        printf("4. Evaluate Performance (with dependency effects)\n");
        printf("5. Show Dependencies (graph)\n");
        printf("6. Add Dependency edge (from -> to)\n");
        printf("7. Exit\n");
        printf("Enter choice: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); /* flush */
            continue;
        }
        while (getchar() != '\n'); /* eat newline */

        switch (choice) {
            case 1:
                displayPerspectives(&g);
                addKPI(&g);
                break;
            case 2:
                displayKPIs(&g);
                break;
            case 3:
                generateScorecard(&g);
                break;
            case 4:
                evaluatePerformanceWithDependencies(&g);
                break;
            case 5:
                showDependencies(&g);
                break;
            case 6:
                displayPerspectives(&g);
                printf("Enter source perspective (from): ");
                if (!fgets(a, sizeof(a), stdin)) break;
                a[strcspn(a, "\r\n")] = '\0';
                printf("Enter destination perspective (to): ");
                if (!fgets(b, sizeof(b), stdin)) break;
                b[strcspn(b, "\r\n")] = '\0';
                addPerspectiveIfNotExists(&g, a);
                addPerspectiveIfNotExists(&g, b);
                addDependency(&g, a, b);
                break;
            case 7:
                printf("Exiting. Freeing memory.\n");
                freeAll(&g);
                return 0;
            default:
                printf("Invalid choice.\n");
        }
    }
    return 0;
}
