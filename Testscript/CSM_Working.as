void main(ScriptEngine@ e)
{
    // ==================================================
    // PLATFORM PROFILE
    // ==================================================

    ProfileCategaory@ platformProfile =
        e.getProfileByName("Platform");


    // ==================================================
    // JET 1 : AGRA -> NEW DELHI
    // RADIO
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

    // Add Radio directly
    e.addRadioSubComponent(jet1, "Radio1");

    Print("Jet1 created with Radio1");


    // ==================================================
    // JET 2 : GURUGRAM -> HATHRAS
    // CSM
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

    // Add CSM directly
    e.addSensorSubComponent(
        jet2,
        "CSM1",
        "CSM"
    );

    Print("Jet2 created with CSM1");


    // ==================================================
    // GET CSM ONLY FOR DETECTION CHECK
    // ==================================================

    Sensor@ jet2CSM =
        jet2.getSensorByName("CSM1");

    if (jet2CSM !is null)
    {
        e.selectEntityDisplay(jet2CSM);

        Print("CSM display selected");

        e.sleep(1000);
    }


    // ==================================================
    // START SIMULATION
    // ==================================================

    SimStart(e);

    Print("=======================================");
    Print("Simulation Started");
    Print("CSM listening for Radio communications");
    Print("=======================================");


    // ==================================================
    // MONITOR CSM
    // ==================================================

    for (int t = 0; t < 400; t++)
    {
        e.sleep(1000);

        if (jet2CSM !is null)
        {
            int csmCount =
                jet2CSM.getCSMTargetCount();

            if (t % 10 == 0)
            {
                Print(
                    "[t=" + t +
                    "] CSM targets: " +
                    csmCount
                );
            }

            if (csmCount > 0)
            {
                Print("=======================================");
                Print("CSM DETECTED RADIO!");
                Print("Target Count: " + csmCount);
                Print("=======================================");

                e.selectEntityDisplay(jet2CSM);

                e.sleep(1000);

                e.captureSensorScreenshot(
                    "csm_detection.png"
                );

                Print("Screenshot captured");

                e.generatePDFReport(
                    "CSM_Detection_Report.pdf"
                );

                Print("PDF report generated");

                Print("Stopping simulation at t=" + t);

                break;
            }
        }
    }


    // ==================================================
    // END SIMULATION
    // ==================================================

    SimPause(e);

    Print("=======================================");
    Print("Simulation Finished");
    Print("=======================================");
}