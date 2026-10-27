void main(ScriptEngine@ e)
{
    ProfileCategaory@ platformProfile =
        e.getProfileByName("Platform");


    // ==================================================
    // Jet1: Agra -> New Delhi
    // HAS RADAR - EMITS SIGNALS
    // ==================================================

    Platform@ jet1 =
        e.addEntityToPlatform(platformProfile, "Jet1");

    jet1.dynamicModel.moveSpeed = 1500;

    jet1.transform.setGeoCord(
        27.1767f,   // Latitude
        78.0081f,   // Longitude
        0.0f        // Altitude
    );

    jet1.trajectory.addWaypoint(
        27.1767f,   // Agra Latitude
        78.0081f,   // Agra Longitude
        0.0f        // Altitude
    );

    jet1.trajectory.addWaypoint(
        28.6139f,   // New Delhi Latitude
        77.2090f,   // New Delhi Longitude
        0.0f        // Altitude
    );

    // Add Radar
    e.addSensorSubComponent(
        jet1,
        "Radar",
        "Generic"
    );

    Print("Jet1 equipped with Radar");


    // ==================================================
    // Jet2: Gurugram -> Hathras
    // HAS ESM - DETECTS RADAR
    // ==================================================

    Platform@ jet2 =
        e.addEntityToPlatform(platformProfile, "Jet2");

    jet2.dynamicModel.moveSpeed = 1500;

    jet2.transform.setGeoCord(
        28.4595f,   // Latitude
        77.0266f,   // Longitude
        0.0f        // Altitude
    );

    jet2.trajectory.addWaypoint(
        28.4595f,   // Gurugram Latitude
        77.0266f,   // Gurugram Longitude
        0.0f        // Altitude
    );

    jet2.trajectory.addWaypoint(
        27.5983f,   // Hathras Latitude
        78.0506f,   // Hathras Longitude
        0.0f        // Altitude
    );

    // Add ESM
    e.addSensorSubComponent(
        jet2,
        "ESM1",
        "ESM"
    );

    Print("Jet2 equipped with ESM1");


    // ==================================================
    // SHOW ESM DISPLAY
    // ==================================================

    Sensor@ jet2ESM =
        jet2.getSensorByName("ESM1");

    if (jet2ESM !is null)
    {
        e.selectEntityDisplay(jet2ESM);

        Print("ESM display selected");
        Print("Jet2 selected in hierarchy");

        e.sleep(1000);
    }


    // ==================================================
    // START SIMULATION
    // ==================================================

    SimStart(e);

    Print("=======================================");
    Print("Simulation started!");
    Print("ESM listening for radar emissions...");
    Print("=======================================");


    bool detectionMade = false;


    // ==================================================
    // ESM DETECTION LOOP
    // ==================================================

    for (int t = 0; t < 400; t++)
    {
        e.sleep(1000);

        if (jet2ESM !is null && !detectionMade)
        {
            int esmCount =
                jet2ESM.getESMTargetCount();

            if (t % 10 == 0)
            {
                Print(
                    "[t=" + t +
                    "] ESM targets: " +
                    esmCount
                );
            }

            // ESM DETECTED RADAR
            if (esmCount > 0)
            {
                detectionMade = true;

                Print("=======================================");
                Print("ESM DETECTED RADAR EMISSION!");
                Print("=======================================");

                Print(
                    "Detection count: " +
                    esmCount
                );

                e.selectEntityDisplay(jet2ESM);

                Print("Switched to ESM display");

                e.sleep(1000);

                e.captureSensorScreenshot(
                    "esm_detection.png"
                );

                Print("ESM detection screenshot captured!");

                Print(
                    "Stopping simulation at t = " +
                    t +
                    " seconds."
                );

                Print("Generating PDF report...");

                e.generatePDFReport(
                    "ESM_Detection_Report.pdf"
                );

                Print("Report generated!");

                Print("=======================================");

                break;
            }
        }
    }


    // ==================================================
    // END SIMULATION
    // ==================================================

    SimPause(e);

    Print("=======================================");
    Print("Simulation finished!");
    Print("Check the PDF for complete evidence.");
    Print("=======================================");
}