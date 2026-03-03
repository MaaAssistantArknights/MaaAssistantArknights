namespace MAAUnified.Tests;

public sealed class BaselineRenderSyncTests
{
    [Fact]
    public void GeneratedDocs_ShouldMatchMachineReadableSource()
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var acceptance = BaselineTestSupport.LoadAcceptanceTemplate();

        var expectedBaselineDoc = BaselineTestSupport.NormalizeLineEndings(BaselineTestSupport.RenderBaselineMarkdown(baseline));
        var expectedAcceptanceDoc = BaselineTestSupport.NormalizeLineEndings(BaselineTestSupport.RenderAcceptanceMarkdown(acceptance));

        var actualBaselineDoc = BaselineTestSupport.ReadDoc("baseline.freeze.v1.md");
        var actualAcceptanceDoc = BaselineTestSupport.ReadDoc("acceptance.checklist.template.v1.md");

        Assert.Equal(expectedBaselineDoc, actualBaselineDoc);
        Assert.Equal(expectedAcceptanceDoc, actualAcceptanceDoc);
    }

    [Fact]
    public void ChangeControlDoc_ShouldContainRequiredSections()
    {
        var doc = BaselineTestSupport.ReadDoc("baseline-change-control.v1.md");

        Assert.Contains("# MAAUnified 基线冻结变更控制 v1", doc, StringComparison.Ordinal);
        Assert.Contains("## 冻结规则", doc, StringComparison.Ordinal);
        Assert.Contains("## Waiver 规则", doc, StringComparison.Ordinal);
        Assert.Contains("## 文档同步", doc, StringComparison.Ordinal);
    }
}
