import argparse, yaml
from agent.orchestrator import Orchestrator

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", default="config.yaml")
    parser.add_argument("--mode", choices=["analyze","plan","fix","full"], default="full")
    args = parser.parse_args()

    with open(args.config, "r", encoding="utf-8") as f:
        cfg = yaml.safe_load(f)

    oc = Orchestrator(cfg)
    if args.mode == "analyze":
        oc.analyze()
    elif args.mode == "plan":
        oc.plan()
    elif args.mode == "fix":
        oc.fix()
    else:
        oc.full()

if __name__ == "__main__":
    main()