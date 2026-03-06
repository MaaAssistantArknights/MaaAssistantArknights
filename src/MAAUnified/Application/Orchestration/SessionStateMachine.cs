namespace MAAUnified.Application.Orchestration;

public enum SessionState
{
    Idle = 0,
    Connecting = 1,
    Connected = 2,
    Running = 3,
    Stopping = 4,
}

public sealed class SessionStateMachine
{
    private static readonly IReadOnlyDictionary<SessionState, HashSet<SessionState>> AllowedTransitions =
        new Dictionary<SessionState, HashSet<SessionState>>
        {
            [SessionState.Idle] =
            [
                SessionState.Connecting,
                SessionState.Connected,
                SessionState.Running,
            ],
            [SessionState.Connecting] =
            [
                SessionState.Idle,
                SessionState.Connected,
                SessionState.Running,
            ],
            [SessionState.Connected] =
            [
                SessionState.Idle,
                SessionState.Connecting,
                SessionState.Running,
            ],
            [SessionState.Running] =
            [
                SessionState.Idle,
                SessionState.Connected,
                SessionState.Stopping,
            ],
            [SessionState.Stopping] =
            [
                SessionState.Idle,
                SessionState.Connected,
                SessionState.Running,
            ],
        };

    public SessionState CurrentState { get; private set; } = SessionState.Idle;

    public event Action<SessionState>? StateChanged;

    public bool TryMoveTo(SessionState state)
    {
        if (CurrentState == state)
        {
            return true;
        }

        if (!AllowedTransitions.TryGetValue(CurrentState, out var allowedStates)
            || !allowedStates.Contains(state))
        {
            return false;
        }

        CurrentState = state;
        StateChanged?.Invoke(state);
        return true;
    }

    public void MoveTo(SessionState state)
    {
        _ = TryMoveTo(state);
    }
}
