#include <atomic>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

// An atomic_compare_exchange wrapper with semantics expected by the paper's
// mutex - return the old value stored in the atom.
inline int cmpxchg(std::atomic<int> *atom, int expected, int desired) {
    int *ep = &expected;
    std::atomic_compare_exchange_strong(atom, ep, desired);
    return *ep;
}

class Mutex {
  public:
    Mutex() : atom_(0) {}

    void lock() {
        int c = cmpxchg(&atom_, 0, 1);
        // If the lock was previously unlocked, there's nothing else for us to
        // do. Otherwise, we'll probably have to wait.
        if (c != 0) {
            do {
                // If the mutex is locked, we signal that we're waiting by
                // setting the
                // atom to 2. A shortcut checks is it's 2 already and avoids the
                // atomic operation in this case.
                if (c == 2 || cmpxchg(&atom_, 1, 2) != 0) {
                    // Here we have to actually sleep, because the mutex is
                    // actually
                    // locked. Note that it's not necessary to loop around this
                    // syscall; a spurious wakeup will do no harm since we only
                    // exit the do...while loop when atom_ is indeed 0.
                    syscall(SYS_futex, (int *)&atom_, FUTEX_WAIT, 2, 0, 0, 0);
                }
                // We're here when either:
                // (a) the mutex was in fact unlocked (by an intervening thread).
                // (b) we slept waiting for the atom and were awoken.
                // So we try to lock the atom again. We set teh state to 2
                // because we can't be certain there's no other thread at this
                // exact point. So we prefer to err on the safe side.
            } while ((c = cmpxchg(&atom_, 0, 2)) != 0);
        }
    }

    void unlock() {
        if (atom_.fetch_sub(1) != 1) {
            atom_.store(0);
            syscall(SYS_futex, (int *)&atom_, FUTEX_WAKE, 1, 0, 0, 0);
        }
    }

  private:
    // 0 means unlocked
    // 1 means locked, no waiters
    // 2 means locked, there are waiters in lock()
    std::atomic<int> atom_;
};