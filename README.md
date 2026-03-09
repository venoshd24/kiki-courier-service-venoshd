# Kiki's Courier Service

A command-line application that estimates delivery costs and delivery times
for a small courier service, built as part of the Everest Engineering coding challenge.

---

## How to Run the App

No installation needed. Use **OnlineGDB**:

1. Go to https://onlinegdb.com/online_c_compiler
2. Delete all the default code
3. Paste the full contents of `kiki-courier-service.c`
4. Click the green **Run** button
5. Type your input in the terminal at the bottom
6. To make it easier, copy the inputs from one of the test cases below
7. Go to the input section of **OnlineGDB**, select **Text** in **Standard Input**
8. Paste the input that has been copied earlier
9. Click the green **Run** button and observe the output
---

## How to Run Tests

The test suite (`test-kiki-courier-service.c`) runs automatically with no input needed:

1. Go to https://onlinegdb.com/online_c_compiler
2. Delete all default code
3. Paste the full contents of `test-kiki-courier-service.c`
4. Click **Run**

Expected: `42 passed, 0 failed`

---

## Test Cases

### Test 1 — Offers not met

```
Input:
100 3
PKG1 5 5 OFR001
PKG2 15 5 OFR002
PKG3 10 100 OFR003
n

Output:
PKG1 0 175
PKG2 0 275
PKG3 35 665
```
PKG1: weight 5kg fails OFR001 (needs 70–200kg). PKG2: weight 15kg fails OFR002 (needs 100–250kg).
PKG3 meets OFR003 (weight 10kg in 10–150, dist 100km in 50–250) — 5% of 700 = 35 discount.

---

### Test 2 — All 3 offers valid

```
Input:
100 3
PKG1 100 99 OFR001
PKG2 200 100 OFR002
PKG3 50 100 OFR003
n

Output:
PKG1 159 1435
PKG2 182 2418
PKG3 55 1045
```
OFR001: 10% of 1595 = 159. OFR002: 7% of 2600 = 182. OFR003: 5% of 1100 = 55.

---

### Test 3 — No offer codes (NA)

```
Input:
100 3
PKG1 10 50 NA
PKG2 20 100 NA
PKG3 30 200 NA
n

Output:
PKG1 0 450
PKG2 0 800
PKG3 0 1400
```
NA always gives zero discount.

---

### Test 4 — Unknown/invalid offer codes

```
Input:
100 2
PKG1 80 100 OFR999
PKG2 120 80 OFR000
n

Output:
PKG1 0 1400
PKG2 0 1700
```
Unrecognised codes silently give zero discount.

---

### Test 5 — Single vehicle, trips one at a time

```
Input:
100 3
PKG1 100 50 NA
PKG2 100 100 NA
PKG3 100 30 NA
y
1
50
100

Output:
PKG1 0 1350 2.20
PKG2 0 1600 5.20
PKG3 0 1250 0.60
```
Each package is 100kg, max load is 100kg — only 1 per trip.
PKG3 goes first (closest), returns fastest, then PKG1, then PKG2.

---

### Test 6 — 1 vehicle, 2 packages fit together

```
Input:
100 3
PKG1 50 50 NA
PKG2 50 100 NA
PKG3 50 30 NA
y
1
50
150

Output:
PKG1 0 850 1.00
PKG2 0 1100 2.00
PKG3 0 750 0.60
```
PKG1+PKG3 = 100kg fit together (max 150kg), delivered first.
PKG3 is closer so it arrives at 0.60, PKG1 at 1.00. Then PKG2 alone on trip 2.

---

### Test 7 — 3 vehicles, all deliver simultaneously

```
Input:
100 3
PKG1 10 30 NA
PKG2 20 50 NA
PKG3 30 20 NA
y
3
100
200

Output:
PKG1 0 350 0.30
PKG2 0 550 0.50
PKG3 0 500 0.20
```
One vehicle per package, all depart at t=0.

---

### Test 8 — Full problem 2 sample (from spec)

```
Input:
100 5
PKG1 50 30 OFR001
PKG2 75 125 OFR008
PKG3 175 100 OFR003
PKG4 110 60 OFR002
PKG5 155 95 NA
y
2
70
200

Output:
PKG1 0 750 3.98
PKG2 0 1475 1.78
PKG3 0 2350 1.42
PKG4 105 1395 0.85
PKG5 0 2125 4.19
```
OFR008 is unknown so PKG2 gets no discount.
PKG4 meets OFR002 (weight 110kg in 100–250, dist 60km in 50–150) — 7% of 1500 = 105.

---

## How the Code is Structured

| Function | Responsibility |
|---|---|
| `getDiscount()` | Only cost logic |
| `bestShipment()` | Only scheduling logic, picks best package combination via bitmask |
| `scheduleDeliveries()` | Assigns delivery times across the fleet |
| `readPackages()` | Input reading and validation only |
| `main()` | Compiles everything together, handles output |

---

## Assumptions & Tradeoffs

**Cost formula** (from spec):
```
delivery cost = base + (weight x 10) + (distance x 5)
```

**Discount** is applied only when BOTH distance AND weight fall within the
offer range. Discount is floored (not rounded) — so 35.7 becomes 35, not 36.

**Adding new offer codes** Adding a new offer = one new row in the
`offers[]` table at the top of `courier.c`. No logic changes needed anywhere
else (Open/Closed principle).

**Shipment selection** tries every combination via bitmask. Priority:
most packages -> heaviest total weight -> shortest max-distance on ties
(vehicle returns sooner). Works well up to 20 packages. For larger inputs
a knapsack DP approach would scale better.

**Vehicle return time** = `currentTime + 2 x floor(maxDist / speed, 2dp)`.

**MAX_PKG = 20:** Bitmask uses `1 << poolSize` — above 20 it gets slow.
The app rejects input above this with a clear error message.

---

## What I'd Do With More Time

- Replace bitmask search with DP knapsack for larger package counts
- Set up GitHub Actions to run tests automatically on every push

---

## Test Coverage (42 tests)

| Test | What it covers |
|---|---|
| Test 1 — Offers not met | Criteria validation per offer |
| Test 2 — All offers valid | OFR001/002/003 correct discount amounts |
| Test 3 — NA codes | NA always gives zero discount |
| Test 4 — Unknown codes | Unrecognised codes give zero discount |
| Test 5 — Single vehicle | One pkg per trip, correct ordering |
| Test 6 — Two pkgs together | Correct grouping and delivery times |
| Test 7 — Simultaneous | Multiple vehicles all depart at t=0 |
| Test 8 — Full spec sample | Matches official example exactly |
| OFR001 boundary | dist=199 qualifies, dist=200 does not |
| Discount floored | 35.5 floors to 35, not 36 |
| floor_to_2dp utility | 3.456->3.45, 3.999->3.99 |

---

## AI Disclosure (Claude)

I built both solutions myself, the cost formula, discount logic, and scheduling algorithm are my own work.
I used AI as a refactoring tool: once my solution was working, I used it to help restructure the code into focused functions (separating core logic from I/O),
tighten up input validation.
I also used it to help with syntax in a few places for Problem 2 since I have not used bitmask before. 
I also generated more test cases aside from those I have written manually and an automated test suite to test the test cases generated.
The README was also AI-assisted.
