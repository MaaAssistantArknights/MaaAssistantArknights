using System;

namespace MaaWpfGui.States
{
    public class RunningState
    {
        private static RunningState instance;

        private RunningState()
        {
        }

        public static RunningState Instance
        {
            get
            {
                instance ??= new RunningState();
                return instance;
            }
        }

        // values
        private bool _idle = true;

        public bool Idle
        {
            get => _idle;
            set
            {
                if (_idle != value)
                {
                    _idle = value;
                    OnIdleChanged(value);
                }
            }
        }

        // getters
        public bool GetIdle() => Idle;

        // action
        public void SetIdle(bool idle) => Idle = idle;

        // subscribes
        public event EventHandler<bool> IdleChanged;

        public virtual void OnIdleChanged(bool newIdleValue)
        {
            IdleChanged?.Invoke(this, newIdleValue);
        }
    }
}
