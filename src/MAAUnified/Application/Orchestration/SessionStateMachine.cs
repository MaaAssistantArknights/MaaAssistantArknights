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
    public SessionState CurrentState { get; private set; } = SessionState.Idle;

    public event Action<SessionState>? StateChanged;

    public void MoveTo(SessionState state)
    {
        if (CurrentState == state)
        {
            return;
        }

        CurrentState = state;
        StateChanged?.Invoke(state);
    }
}
