/*
 * Automated test suite for kiki-courier-service
 * Paste into https://onlinegdb.com/online_c_compiler and click Run
 * No input needed, tests run automatically.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#define MAX_PKG      20
#define MAX_VEH      10
#define WEIGHT_RATE  10.0
#define DIST_RATE     5.0

typedef struct {
    char   code[20];
    double pct;
    double minDist, maxDist;
    double minKg, maxKg;
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
    double eta;
} Pkg;

static double f2(double n) { return floor(n * 100.0) / 100.0; }

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

static int bestShipment(Pkg** pool, int sz, double maxLoad) {
    int    mask, j, best = 0, bestCnt = 0;
    double bestKg = 0.0, bestDist = 1e18;
    for (mask = 1; mask < (1 << sz); mask++) {
        double kg = 0.0, md = 0.0; int cnt = 0;
        for (j = 0; j < sz; j++) {
            if (!(mask & (1 << j))) continue;
            kg += pool[j]->kg; cnt++;
            if (pool[j]->dist > md) md = pool[j]->dist;
        }
        if (kg > maxLoad) continue;
        if (cnt > bestCnt
            || (cnt == bestCnt && kg > bestKg)
            || (cnt == bestCnt && kg == bestKg && md < bestDist))
            { best = mask; bestCnt = cnt; bestKg = kg; bestDist = md; }
    }
    return best;
}

static void scheduleDeliveries(Pkg* pkgs, int n,
                                int vehicles, double speed, double maxLoad) {
    double vtime[MAX_VEH]; int done[MAX_PKG];
    Pkg* pool[MAX_PKG];    int pidx[MAX_PKG];
    int i, j, left, v, sz, mask; double t, md;
    for (i = 0; i < vehicles; i++) vtime[i] = 0.0;
    for (i = 0; i < n; i++)        done[i]  = 0;
    left = n;
    while (left > 0) {
        v = 0;
        for (i = 1; i < vehicles; i++) if (vtime[i] < vtime[v]) v = i;
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
            done[pidx[j]] = 1; left--;
        }
        vtime[v] = t + 2.0 * f2(md / speed);
    }
}

static int passed = 0, failed = 0;

static void check(const char* label, int ok) {
    if (ok) { printf("  PASS: %s\n", label); passed++; }
    else     { printf("  FAIL: %s\n", label); failed++; }
}
static void checkD(const char* label, double exp, double got) {
    check(label, fabs(exp - got) < 0.001);
}

static Pkg pkg(const char* id, double kg, double dist, const char* code) {
    Pkg p;
    strncpy(p.id, id, 19);          p.id[19] = '\0';
    strncpy(p.offerCode, code, 19); p.offerCode[19] = '\0';
    p.kg = kg; p.dist = dist; p.eta = -1.0;
    return p;
}

static double cost(double base, double kg, double dist) {
    return base + kg * WEIGHT_RATE + dist * DIST_RATE;
}

/* COST TESTS */

static void test_t1_offers_not_met(void) {
    /* PKG1: weight 5kg < OFR001 min 70kg no discount
       PKG2: weight 15kg < OFR002 min 100kg no discount
       PKG3: weight 10kg & dist 100km  meets OFR003 */
    Pkg p1 = pkg("PKG1",  5,   5, "OFR001");
    Pkg p2 = pkg("PKG2", 15,   5, "OFR002");
    Pkg p3 = pkg("PKG3", 10, 100, "OFR003");
    double r1 = cost(100, p1.kg, p1.dist); /* 175 */
    double r2 = cost(100, p2.kg, p2.dist); /* 275 */
    double r3 = cost(100, p3.kg, p3.dist); /* 700 */
    printf("\n[Test 1] Offers not met\n");
    checkD("PKG1 discount = 0",    0.0, getDiscount(&p1, r1));
    checkD("PKG1 total = 175",   175.0, r1 - getDiscount(&p1, r1));
    checkD("PKG2 discount = 0",    0.0, getDiscount(&p2, r2));
    checkD("PKG2 total = 275",   275.0, r2 - getDiscount(&p2, r2));
    checkD("PKG3 discount = 35",  35.0, getDiscount(&p3, r3));
    checkD("PKG3 total = 665",   665.0, r3 - getDiscount(&p3, r3));
}

static void test_t2_all_offers_valid(void) {
    Pkg p1 = pkg("PKG1", 100,  99, "OFR001"); /* raw=1595, 10% -> 159 */
    Pkg p2 = pkg("PKG2", 200, 100, "OFR002"); /* raw=2600,  7% -> 182 */
    Pkg p3 = pkg("PKG3",  50, 100, "OFR003"); /* raw=1100,  5% ->  55 */
    double r1 = cost(100, p1.kg, p1.dist);
    double r2 = cost(100, p2.kg, p2.dist);
    double r3 = cost(100, p3.kg, p3.dist);
    printf("\n[Test 2] All 3 offers valid\n");
    checkD("PKG1 discount = 159.5",  159.5, getDiscount(&p1, r1));
    checkD("PKG1 total = 1435.5",   1435.5, r1 - getDiscount(&p1, r1));
    checkD("PKG2 discount = 182",  182.0, getDiscount(&p2, r2));
    checkD("PKG2 total = 2418",   2418.0, r2 - getDiscount(&p2, r2));
    checkD("PKG3 discount = 55",    55.0, getDiscount(&p3, r3));
    checkD("PKG3 total = 1045",   1045.0, r3 - getDiscount(&p3, r3));
}

static void test_t3_no_offer_codes(void) {
    Pkg p1 = pkg("PKG1", 10,  50, "NA");
    Pkg p2 = pkg("PKG2", 20, 100, "NA");
    Pkg p3 = pkg("PKG3", 30, 200, "NA");
    printf("\n[Test 3] NA offer codes  no discount\n");
    checkD("PKG1 discount = 0",  0.0, getDiscount(&p1, cost(100,p1.kg,p1.dist)));
    checkD("PKG2 discount = 0",  0.0, getDiscount(&p2, cost(100,p2.kg,p2.dist)));
    checkD("PKG3 discount = 0",  0.0, getDiscount(&p3, cost(100,p3.kg,p3.dist)));
    checkD("PKG1 total = 450",   450.0, cost(100,p1.kg,p1.dist));
    checkD("PKG2 total = 800",   800.0, cost(100,p2.kg,p2.dist));
    checkD("PKG3 total = 1400", 1400.0, cost(100,p3.kg,p3.dist));
}

static void test_t4_unknown_offer_codes(void) {
    Pkg p1 = pkg("PKG1",  80, 100, "OFR999");
    Pkg p2 = pkg("PKG2", 120,  80, "OFR000");
    printf("\n[Test 4] Unknown offer codes  no discount\n");
    checkD("PKG1 discount = 0",  0.0, getDiscount(&p1, cost(100,p1.kg,p1.dist)));
    checkD("PKG2 discount = 0",  0.0, getDiscount(&p2, cost(100,p2.kg,p2.dist)));
    checkD("PKG1 total = 1400", 1400.0, cost(100,p1.kg,p1.dist));
    checkD("PKG2 total = 1700", 1700.0, cost(100,p2.kg,p2.dist));
}

/* SCHEDULER TESTS*/

static void test_t5_single_vehicle_one_per_trip(void) {
    /* 100kg each, max load 100kg -> 1 per trip
       PKG3 goes first (shortest), then PKG1, then PKG2 */
    Pkg pkgs[3];
    pkgs[0] = pkg("PKG1", 100,  50, "NA");
    pkgs[1] = pkg("PKG2", 100, 100, "NA");
    pkgs[2] = pkg("PKG3", 100,  30, "NA");
    scheduleDeliveries(pkgs, 3, 1, 50.0, 100.0);
    printf("\n[Test 5] Single vehicle, one package per trip\n");
    checkD("PKG1 eta = 2.20", 2.20, pkgs[0].eta);
    checkD("PKG2 eta = 5.20", 5.20, pkgs[1].eta);
    checkD("PKG3 eta = 0.60", 0.60, pkgs[2].eta);
}

static void test_t6_two_packages_fit_together(void) {
    /* PKG1+PKG3 = 100kg fit (max 150kg), delivered first
       PKG2 goes on second trip */
    Pkg pkgs[3];
    pkgs[0] = pkg("PKG1", 50,  50, "NA");
    pkgs[1] = pkg("PKG2", 50, 100, "NA");
    pkgs[2] = pkg("PKG3", 50,  30, "NA");
    scheduleDeliveries(pkgs, 3, 1, 50.0, 150.0);
    printf("\n[Test 6] 1 vehicle, 2 packages fit together first trip\n");
    checkD("PKG1 eta = 1.00", 1.00, pkgs[0].eta);
    checkD("PKG2 eta = 2.00", 2.00, pkgs[1].eta);
    checkD("PKG3 eta = 0.60", 0.60, pkgs[2].eta);
}

static void test_t7_three_vehicles_simultaneous(void) {
    /* 3 vehicles, 3 packages  all depart at t=0 */
    Pkg pkgs[3];
    pkgs[0] = pkg("PKG1", 10, 30, "NA");
    pkgs[1] = pkg("PKG2", 20, 50, "NA");
    pkgs[2] = pkg("PKG3", 30, 20, "NA");
    scheduleDeliveries(pkgs, 3, 3, 100.0, 200.0);
    printf("\n[Test 7] 3 vehicles, all deliver simultaneously\n");
    checkD("PKG1 eta = 0.30", 0.30, pkgs[0].eta);
    checkD("PKG2 eta = 0.50", 0.50, pkgs[1].eta);
    checkD("PKG3 eta = 0.20", 0.20, pkgs[2].eta);
}

static void test_t8_full_problem2_sample(void) {
    /* Official spec sample */
    Pkg pkgs[5];
    pkgs[0] = pkg("PKG1",  50,  30, "OFR001");
    pkgs[1] = pkg("PKG2",  75, 125, "OFR008");
    pkgs[2] = pkg("PKG3", 175, 100, "OFR003");
    pkgs[3] = pkg("PKG4", 110,  60, "OFR002");
    pkgs[4] = pkg("PKG5", 155,  95, "NA");
    scheduleDeliveries(pkgs, 5, 2, 70.0, 200.0);
    printf("\n[Test 8] Full problem 2 sample\n");
    checkD("PKG1 eta = 3.98", 3.98, pkgs[0].eta);
    checkD("PKG2 eta = 1.78", 1.78, pkgs[1].eta);
    checkD("PKG3 eta = 1.42", 1.42, pkgs[2].eta);
    checkD("PKG4 eta = 0.85", 0.85, pkgs[3].eta);
    checkD("PKG5 eta = 4.19", 4.19, pkgs[4].eta);
}

/* EDGE CASE TESTS� */

static void test_offer_boundary_values(void) {
    /* OFR001: dist must be < 200 (strictly), weight 70-200 */
    Pkg atBoundary  = pkg("P1", 100, 199, "OFR001"); /* just inside  */
    Pkg overBoundary = pkg("P2", 100, 200, "OFR001"); /* just outside */
    double r1 = cost(100, atBoundary.kg,   atBoundary.dist);
    double r2 = cost(100, overBoundary.kg, overBoundary.dist);
    printf("\n[Edge] OFR001 distance boundary\n");
    check("dist=199 qualifies",     getDiscount(&atBoundary,   r1) > 0);
    checkD("dist=200 no discount", 0.0, getDiscount(&overBoundary, r2));
}

static void test_discount_floored_not_rounded(void) {
    /* 5% of 710 = 35.5 -> floor to 35, not 36 */
    Pkg p = pkg("P1", 11, 100, "OFR003"); /* raw = 100+110+500 = 710 */
    double raw = cost(100, p.kg, p.dist);
    printf("\n[Edge] Discount floored not rounded\n");
    checkD("floor(710 * 5%) = 35.5", 35.5, getDiscount(&p, raw));
}

static void test_floor2_utility(void) {
    printf("\n[Edge] floor to 2dp\n");
    checkD("3.456 -> 3.45", 3.45, f2(3.456));
    checkD("3.999 -> 3.99", 3.99, f2(3.999));
    checkD("1.785 -> 1.78", 1.78, f2(1.785));
}

/* main  */
int main(void) {
    printf("================================\n");
    printf(" Kiki's Courier Test Suite\n");
    printf("================================\n");

    test_t1_offers_not_met();
    test_t2_all_offers_valid();
    test_t3_no_offer_codes();
    test_t4_unknown_offer_codes();
    test_t5_single_vehicle_one_per_trip();
    test_t6_two_packages_fit_together();
    test_t7_three_vehicles_simultaneous();
    test_t8_full_problem2_sample();
    test_offer_boundary_values();
    test_discount_floored_not_rounded();
    test_floor2_utility();

    printf("\n================================\n");
    printf(" %d passed, %d failed\n", passed, failed);
    printf("================================\n");
    return failed > 0 ? 1 : 0;
}
