namespace MAAUnified.Application.Models;

public sealed record UiLogMessage(DateTimeOffset Timestamp, string Level, string Message);
