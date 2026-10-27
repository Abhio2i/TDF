void main(ScriptEngine@ e)
{
    // Add layer
    e.AddLayer("Layer1");

    // Start line drawing
    e.StartLine();

    // Latitude, Longitude
    e.AddLinePoint(28.08, 73.38);
    e.AddLinePoint(26.99, 75.79);

    // Finish line
    e.FinishLine();

    // Generate PDF report
    e.generatePDFReport("line_pdf.pdf");
}