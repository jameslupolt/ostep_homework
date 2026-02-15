## Definitions you’ll use for every question

Assumptions (from the chapter’s standard setup): all jobs arrive at **time 0**, single CPU, no I/O, ignore context-switch overhead.

* **Response time** for job *i*:
  ( R_i = t^{start}_i - t^{arrival}_i ) (time units)

* **Turnaround time** for job *i*:
  ( T_i = t^{finish}_i - t^{arrival}_i ) (time units)

Since (t^{arrival}_i = 0) here, these simplify to:

* (R_i = t^{start}_i)
* (T_i = t^{finish}_i)

**Type:** Inferred (standard scheduling metric definitions)
**Confidence:** 10/10

---

## Q1) Three jobs, each length 200: FIFO and SJF

Let the jobs be A, B, C with lengths (200,200,200) (time units).
With equal lengths, **SJF can’t improve anything** (ties), so **FIFO and SJF produce the same turnaround/response set**.

### FIFO (and SJF)

Timeline:

* A runs [0,200]
* B runs [200,400]
* C runs [400,600]

Per job:

| Job | Length | Start | Finish | Response (R) | Turnaround (T) |
| --- | -----: | ----: | -----: | -----------: | -------------: |
| A   |    200 |     0 |    200 |            0 |            200 |
| B   |    200 |   200 |    400 |          200 |            400 |
| C   |    200 |   400 |    600 |          400 |            600 |

Averages:

* (R_{avg} = (0 + 200 + 400) / 3 = 600/3 = 200)
* (T_{avg} = (200 + 400 + 600) / 3 = 1200/3 = 400)

**Type:** Inferred (direct computation)
**Confidence:** 10/10

---

## Q2) Three jobs with lengths 100, 200, 300: FIFO and SJF

Here the answer depends on **the FIFO arrival order** (because FIFO uses arrival order; SJF sorts by length).

### Assumption A (common in the simulator): FIFO order is exactly the list you give, i.e. 100 then 200 then 300

Then FIFO already runs shortest→longest, so **FIFO = SJF**.

Timeline:

* A(100): [0,100]
* B(200): [100,300]
* C(300): [300,600]

| Job | Length | Start | Finish | Response (R) | Turnaround (T) |
| --- | -----: | ----: | -----: | -----------: | -------------: |
| A   |    100 |     0 |    100 |            0 |            100 |
| B   |    200 |   100 |    300 |          100 |            300 |
| C   |    300 |   300 |    600 |          300 |            600 |

Averages:

* (R_{avg} = (0 + 100 + 300)/3 = 400/3 \approx 133.33)
* (T_{avg} = (100 + 300 + 600)/3 = 1000/3 \approx 333.33)

**Type:** Inferred (direct computation)
**Confidence:** 9/10 (depends on arrival/order assumption)

### Alternative hypothesis (also reasonable): FIFO arrival order is different (e.g., 300 then 200 then 100)

This is the “convoy effect” case where FIFO is worse and SJF helps.

FIFO order 300→200→100:

| Job | Length | Start | Finish | Response (R) | Turnaround (T) |
| --- | -----: | ----: | -----: | -----------: | -------------: |
| A   |    300 |     0 |    300 |            0 |            300 |
| B   |    200 |   300 |    500 |          300 |            500 |
| C   |    100 |   500 |    600 |          500 |            600 |

Averages:

* (R_{avg} = (0 + 300 + 500)/3 = 800/3 \approx 266.67)
* (T_{avg} = (300 + 500 + 600)/3 = 1400/3 \approx 466.67)

SJF would reorder to 100→200→300 and give the better averages from Assumption A.

**Type:** Inferred (direct computation)
**Confidence:** 9/10

---

## Q3) Same workloads, but with RR, time-slice (quantum) = 1

Round Robin gives each job up to **q = 1** time unit before rotating.

### Key response-time fact for RR when all arrive at time 0

In the **first rotation**, jobs get CPU quickly:

* First job: (R = 0)
* Second job: (R = 1)
* Third job: (R = 2)

(Assuming order A,B,C in the queue.)

**Type:** Inferred
**Confidence:** 9/10

### RR(q=1) for lengths 100, 200, 300 (order A=100, B=200, C=300)

* Response times: A=0, B=1, C=2
* Turnaround times (completion times) work out to:

  * A finishes at **298**
  * B finishes at **499**
  * C finishes at **600**

So:

| Job | Length | Response (R) | Turnaround (T) |
| --- | -----: | -----------: | -------------: |
| A   |    100 |            0 |            298 |
| B   |    200 |            1 |            499 |
| C   |    300 |            2 |            600 |

Average turnaround:

* (T_{avg} = (298 + 499 + 600)/3 = 1397/3 \approx 465.67)

Average response:

* (R_{avg} = (0 + 1 + 2)/3 = 1)

**Type:** Inferred (I verified with a small RR simulation)
**Confidence:** 10/10

### (If you also want RR for Q1’s 200,200,200)

RR(q=1) gives:

* Response: 0, 1, 2
* Turnaround: 598, 599, 600

**Type:** Inferred (verified with simulation)
**Confidence:** 10/10

---

## Q4) When does SJF have the same turnaround times as FIFO?

SJF and FIFO match on turnaround **exactly when they run jobs in the same order**.

With the chapter’s “all arrive together” assumption, that happens when:

* All jobs have **equal length**, or
* The FIFO arrival/order is already **sorted shortest → longest** (ties anywhere).

More general version (if arrival times differ): whenever each time the CPU picks the next job, the FIFO-next job is also the shortest among the ready jobs.

**Type:** Inferred (order equivalence condition)
**Confidence:** 9/10

---

## Q5) When does SJF have the same response times as RR?

RR usually improves response time because it gives everyone a slice early. So equality is a “special case”.

A clean condition (matches how the simulator behaves):

* RR will match a non-preemptive run **when no job gets preempted before it finishes**, i.e.
  (q \ge \max_i(L_i)) (quantum at least the longest job length).
  Then RR degenerates into FIFO.

So SJF and RR have the same response times when:

1. (q \ge \max(L_i)) so RR behaves like FIFO, **and**
2. FIFO’s order matches SJF’s order (e.g., jobs already in shortest→longest order, or equal lengths).

**Type:** Inferred
**Confidence:** 8/10 (depends on what workload/order you’re assuming)

A slightly broader way to say it: response times match whenever the **first time each job runs** occurs at the same time in both schedules — which typically requires RR not to preempt any earlier job before completion.

---

## Q6) What happens to response time with SJF as job lengths increase?

Under non-preemptive SJF, if you sort lengths (L_1 \le L_2 \le ... \le L_N), then:

* (R_1 = 0)
* (R_2 = L_1)
* (R_3 = L_1 + L_2)
* In general:
  [
  R_k = \sum_{i=1}^{k-1} L_i \quad \text{(time units)}
  ]

So response time for later jobs grows with the **sum of earlier job lengths** — i.e., it grows **linearly** as those lengths scale up.

Important nuance:

* Making the **shortest** job longer increases *everyone else’s* response time.
* Making only the **longest** job longer usually does **not** change its response time (it still starts after all shorter jobs), but it does increase its **turnaround**.

**Type:** Inferred (formula from schedule structure)
**Confidence:** 9/10

How to demonstrate quickly in the simulator: compare SJF on small vs. scaled-up job lists, e.g.

* `-l 10,20,30` vs `-l 100,200,300`

You should see response times scale roughly by 10× for the affected jobs.

---

## Q7) What happens to response time with RR as quantum increases? Worst-case equation in terms of N

As the quantum (q) increases, RR gives each running job a **longer uninterrupted slice**, so a job waiting for its *first* slice may wait longer.

Worst-case response time (all N jobs ready at time 0, and you are last in the initial RR order):

[
R_{worst} = (N - 1)\cdot q \quad \text{(time units)}
]

That bound is tight when all other jobs run at least one full quantum before you get CPU (i.e., their remaining time (\ge q)).

**Type:** Inferred (worst-case bound)
**Confidence:** 10/10

---

## How to verify with `scheduler.py` (quick commands)

The official `scheduler.py` supports:

* `-p` policy: `FIFO`, `SJF`, `RR`
* `-l` comma-separated job lengths
* `-q` RR quantum
* `-c` compute/print the answers
  (plus `-s` seed, `-j` job count, `-m` max length for random jobs). ([GitHub][1])

Try:

```bash
# Q1
python3 scheduler.py -p FIFO -l 200,200,200 -c
python3 scheduler.py -p SJF  -l 200,200,200 -c

# Q2 (order as listed)
python3 scheduler.py -p FIFO -l 100,200,300 -c
python3 scheduler.py -p SJF  -l 100,200,300 -c

# Q3
python3 scheduler.py -p RR -q 1 -l 100,200,300 -c
```

[1]: https://raw.githubusercontent.com/remzi-arpacidusseau/ostep-homework/master/cpu-sched/scheduler.py "raw.githubusercontent.com"

