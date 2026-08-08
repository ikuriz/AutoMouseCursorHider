namespace AutoMouseCursorHider;

public static class SingleInstance
{
    public static bool TryAcquire(string name, out IDisposable? lease)
    {
        var mutex = new Mutex(initiallyOwned: true, name, out var createdNew);
        if (!createdNew)
        {
            mutex.Dispose();
            lease = null;
            return false;
        }

        lease = new MutexLease(mutex);
        return true;
    }

    private sealed class MutexLease : IDisposable
    {
        private Mutex? _mutex;

        public MutexLease(Mutex mutex) => _mutex = mutex;

        public void Dispose()
        {
            var mutex = Interlocked.Exchange(ref _mutex, null);
            if (mutex is null)
            {
                return;
            }

            try
            {
                mutex.ReleaseMutex();
            }
            finally
            {
                mutex.Dispose();
            }
        }
    }
}
