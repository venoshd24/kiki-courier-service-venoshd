#include <stdio.h>
#include <string.h>
#include <math.h>

#define MAX_PKG  20
#define MAX_VEH  10

/* cost formula multipliers from the problem spec */
#define WEIGHT_RATE   10.0
#define DIST_RATE      5.0

/* -----------------------------------------------------------
   Offer table - add new rows here to support more offer codes
   ----------------------------------------------------------- */
typedef struct {
    char   code[20];
    double pct;
    double minDist, maxDist;
    double minKg,   maxKg;
} Offer;

static Offer offers[] = {
    { "OFR001", 10.0,  0.0, 199.0,  70.0, 200.0 },
    { "OFR002",  7.0, 50.0, 150.0, 100.0, 250.0 },
    { "OFR003",  5.0, 50.0, 250.0,  10.0, 150.0 },
};
static int numOffers = 3;

typedef struct {
    char   id[20];
    double kg, dist;
    char   offerCode[20];
    double eta;   /* delivery time, -1 if not scheduled yet */
} Pkg;

/* floor to 2dp - spec says floor not round e.g. 3.456 -> 3.45 */
static double f2(double n) {
    return floor(n * 100.0) / 100.0;
}

static double getDiscount(const Pkg* p, double raw) {
    int i;
    for (i = 0; i < numOffers; i++) {
        Offer* o = &offers[i];
        if (strcmp(p->offerCode, o->code) == 0
            && p->dist >= o->minDist && p->dist <= o->maxDist
            && p->kg   >= o->minKg   && p->kg   <= o->maxKg)
            return floor(raw * o->pct) / 100.0;
    }
    return 0.0;
}

/*
 * Pick the best group of packages for one vehicle trip.
 * Best means: most packages first, then heaviest, then
 * shortest max-distance if still tied (returns sooner).
 * Uses bitmask to try every possible combination.
 */
static int bestShipment(Pkg** pool, int sz, double maxLoad) {
    int    mask, j, best = 0, bestCnt = 0;
    double bestKg = 0.0, bestDist = 1e18;

    for (mask = 1; mask < (1 << sz); mask++) {
        double kg = 0.0, md = 0.0;
        int cnt = 0;
        for (j = 0; j < sz; j++) {
            if (!(mask & (1 << j))) continue;
            kg += pool[j]->kg; cnt++;
            if (pool[j]->dist > md) md = pool[j]->dist;
        }
        if (kg > maxLoad) continue;
        if (cnt > bestCnt
            || (cnt == bestCnt && kg > bestKg)
            || (cnt == bestCnt && kg == bestKg && md < bestDist)) {
            best = mask; bestCnt = cnt; bestKg = kg; bestDist = md;
        }
    }
    return best;
}

static void scheduleDeliveries(Pkg* pkgs, int n,
                                int vehicles, double speed, double maxLoad) {
    double vtime[MAX_VEH];
    int    done[MAX_PKG];
    Pkg*   pool[MAX_PKG];
    int    pidx[MAX_PKG];
    int    i, j, left, v, sz, mask;
    double t, md;

    for (i = 0; i < vehicles; i++) vtime[i] = 0.0;
    for (i = 0; i < n; i++)        done[i]  = 0;
    left = n;

    while (left > 0) {
        /* find which vehicle is free earliest */
        v = 0;
        for (i = 1; i < vehicles; i++)
            if (vtime[i] < vtime[v]) v = i;
        t = vtime[v];

        sz = 0;
        for (i = 0; i < n; i++)
            if (!done[i]) { pool[sz] = &pkgs[i]; pidx[sz++] = i; }

        mask = bestShipment(pool, sz, maxLoad);
        if (mask == 0) break;

        md = 0.0;
        for (j = 0; j < sz; j++) {
            if (!(mask & (1 << j))) continue;
            pkgs[pidx[j]].eta = f2(t + pool[j]->dist / speed);
            if (pool[j]->dist > md) md = pool[j]->dist;
            done[pidx[j]] = 1;
            left--;
        }
        /* return trip is same distance, so available again after 2x one-way */
        vtime[v] = t + 2.0 * f2(md / speed);
    }
}

static int readPackages(Pkg* pkgs, int n) {
    int i;
    for (i = 0; i < n; i++) {
        pkgs[i].eta = -1.0;
        printf("Package %d - id weight distance offer_code: ", i + 1);
        if (scanf("%19s %lf %lf %19s",
                  pkgs[i].id, &pkgs[i].kg,
                  &pkgs[i].dist, pkgs[i].offerCode) != 4) {
            fprintf(stderr, "Error reading package %d\n", i + 1);
            return 0;
        }
        if (pkgs[i].kg <= 0 || pkgs[i].dist <= 0) {
            fprintf(stderr, "Error: %s has invalid weight or distance\n", pkgs[i].id);
            return 0;
        }
    }
    return 1;
}

int main(void) {
    double base;
    int    n, i, timed = 0;
    Pkg    pkgs[MAX_PKG];
    char   choice;

    printf("Base delivery cost and number of packages: ");
    if (scanf("%lf %d", &base, &n) != 2 || base < 0 || n <= 0 || n > MAX_PKG) {
        fprintf(stderr, "Error: invalid base cost or package count\n");
        return 1;
    }

    if (!readPackages(pkgs, n)) return 1;

    printf("Estimate delivery times? (y/n): ");
    scanf(" %c", &choice);

    if (choice == 'y' || choice == 'Y') {
        int    vehicles;
        double speed, maxLoad;
        printf("Number of vehicles: ");  scanf("%d",  &vehicles);
        printf("Max speed (km/hr): ");   scanf("%lf", &speed);
        printf("Max load (kg): ");       scanf("%lf", &maxLoad);

        if (vehicles <= 0 || vehicles > MAX_VEH || speed <= 0 || maxLoad <= 0) {
            fprintf(stderr, "Error: invalid fleet details\n");
            return 1;
        }
        scheduleDeliveries(pkgs, n, vehicles, speed, maxLoad);
        timed = 1;
    }

    printf("\n--- Results ---\n");
    for (i = 0; i < n; i++) {
        double raw      = base + pkgs[i].kg * WEIGHT_RATE + pkgs[i].dist * DIST_RATE;
        double discount = getDiscount(&pkgs[i], raw);
        printf("%s %lld %lld", pkgs[i].id,
               (long long)discount, (long long)(raw - discount));
        if (timed && pkgs[i].eta >= 0)
            printf(" %.2f", pkgs[i].eta);
        printf("\n");
    }
    return 0;
}
