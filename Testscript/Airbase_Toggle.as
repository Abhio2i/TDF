void main(ScriptEngine@ e)
{
    // Enable Airbase Layer
    e.ToggleAirbases();

    // Generate Auto Report
    e.generatePDFReport("Airbase_India_Report.pdf");
}
