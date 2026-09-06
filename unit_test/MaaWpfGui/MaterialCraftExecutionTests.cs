using System;
using System.Collections.Generic;
using MaaWpfGui.Models;

var targets = new Dictionary<string, int> { ["A"] = 10 };
var execution = new MaterialCraftExecution(42, targets);
targets["A"] = 20;
targets.Clear();
Require(execution.TaskId == 42 && execution.Targets["A"] == 10, "Submitted targets must be a snapshot");
Require(execution.Targets is IDictionary<string, int> { IsReadOnly: true }, "The snapshot cannot be mutated by its caller");
Require(!execution.HasConfirmedCompletion, "A chain completion without operation callbacks is not confirmed output");
Require(!execution.CompleteOperation(0), "Do not apply completion without a started operation");
Require(!execution.BeginOperation(1), "Reject out-of-order operations");
Require(execution.BeginOperation(0), "Start the first operation");
Require(!execution.BeginOperation(0), "Ignore a duplicate start");
Require(!execution.CompleteOperation(1), "Ignore completion for another operation");
Require(execution.PendingOperation == 0, "An interrupted operation remains uncertain");
Require(execution.CompleteOperation(0), "Apply confirmed output even if stop was requested");
Require(!execution.CompleteOperation(0), "Never apply the same inventory delta twice");
Require(execution.CompletedOperations == 1 && execution.PendingOperation is null, "Keep confirmed progress");
Require(execution.HasConfirmedCompletion, "A fully acknowledged operation can finish the chain");
Require(execution.BeginOperation(1), "Continue the next operation");
Require(execution.CompletedOperations == 1 && execution.PendingOperation == 1, "Partial completion retains its prefix");
Require(!execution.HasConfirmedCompletion, "A pending operation prevents a successful finish");
var plannedOutputs = new Dictionary<string, long> { ["A"] = 5, ["B"] = 1 };
var round = new MaterialCraftExecution(43, new Dictionary<string, int> { ["A"] = 2, ["B"] = 1 }, plannedOutputs);
plannedOutputs["A"] = 2;
Require(!round.RecordCraftedOutput("A", 2), "Intermediate production must not complete a target with later batches remaining");
Require(!round.RecordCraftedOutput("A", 1), "Partial batches must keep the target queued");
Require(round.RecordCraftedOutput("A", 2), "Complete the target after all planned batches, using a snapshot of the plan");
Require(!round.RecordCraftedOutput("C", 1), "An intermediate-only item is not a queued target");
Require(!round.RecordCraftedOutput("B", 0), "Zero output cannot finish a target");
Require(round.RecordCraftedOutput("B", 1), "The next target completes independently");
round.RecordInventoryChanges([
    new() { Id = "A", OldCount = 10, NewCount = 15 },
    new() { Id = "C", OldCount = 20, NewCount = 14 },
]);
round.RecordInventoryChanges([
    new() { Id = "A", OldCount = 15, NewCount = 12 },
    new() { Id = "B", OldCount = 0, NewCount = 1 },
    new() { Id = "C", OldCount = 14, NewCount = 20 },
]);
var netChanges = new Dictionary<string, MaterialCraftInventoryChange>();
foreach (var change in round.InventoryChanges) netChanges.Add(change.Id, change);
Require(netChanges.Count == 3, "Merge repeated materials into one round summary");
Require(netChanges["A"].OldCount == 10 && netChanges["A"].NewCount == 12, "Keep round-start stock and final stock after subsequent consumption");
Require(netChanges["B"].OldCount == 0 && netChanges["B"].NewCount == 1, "Include materials first produced later in the round");
Require(netChanges["C"].OldCount == netChanges["C"].NewCount, "Zero net changes can be omitted by the final renderer");
Require(new MaterialCraftExecution(44, targets).InventoryChanges.Count == 0, "A new execution must not inherit the previous round's changes");
Console.WriteLine("MaterialCraftExecution: 27 checks passed");

static void Require(bool condition, string message)
{
    if (!condition)
    {
        throw new Exception(message);
    }
}
