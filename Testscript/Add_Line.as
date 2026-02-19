void main(ScriptEngine@ e) {

    // Start line drawing
    e.canvasStartLine();

    // Add line points
    e.canvasAddLinePoint(73.38, 28.08);
    e.canvasAddLinePoint(75.79, 26.99);

    // Finish line
    e.canvasFinishLine();

    // Generate PDF report
    e.generatePDFReport("line_pdf.pdf");
}
