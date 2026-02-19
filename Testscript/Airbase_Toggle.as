void main(ScriptEngine@ e)
{
    // Enable Airbase Layer
    e.canvasToggleAirbases();

    // Generate Auto Report
    e.generatePDFReport("Airbase_India_Report.pdf");
}
