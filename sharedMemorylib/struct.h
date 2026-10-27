#include <cstdint>
#include <cstring>
#include <variant>
#include <string>

namespace SharedStructs {

// AIS Navigation Status
enum class NavStatus : uint8_t {
    UnderWayEngine = 0,
    AtAnchor = 1,
    NotUnderCommand = 2,
    RestrictedManoeuvrability = 3,
    ConstrainedByDraft = 4,
    Moored = 5,
    Aground = 6,
    EngagedInFishing = 7,
    UnderWaySailing = 8,
    Reserved = 15 // Unknown
};

// Ship and Cargo Types
enum class ShipType : uint8_t {
    Unknown = 0,
    Fishing = 30,
    Tug = 52,
    Passenger = 60,
    Cargo = 70,
    Tanker = 80,
    PleasureCraft = 37
};


using ParamValue = std::variant<int, float, double, std::string, bool>;

// Generic parameter structure for custom configurable values
struct parameter {
private:
    char name[50];          // Name of the parameter
    int dataType;           // 0=int, 1=float, 2=double, 3=string, 4=bool

    // Shared memory ke liye hum sirf primitive types use karenge
    union {
        int valueInt;
        float valueFloat;
        double valueDouble;
        char valueString[100];
        bool valueBool;
    } data;
public:
    void setName(const char* newName) {
        strncpy(name, newName, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0'; // Null-terminate to avoid overflow
    }
    void getName(char* outName) const {
        strncpy(outName, name, sizeof(name) - 1);
        outName[sizeof(name) - 1] = '\0'; // Ensure null termination
    }

    void getDataType(int& outType) const {
        outType = dataType;
    }

    void setDataType(int type) {
        dataType = type;
    }


public:
    void setValue(const ParamValue& val) {
        // Yahan 'std::visit' use karna sabse sahi tarika hai variant ke liye
        std::visit([this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int>) {
                data.valueInt = arg;
                dataType = 0;
            } else if constexpr (std::is_same_v<T, float>) {
                data.valueFloat = arg;
                dataType = 1;
            } else if constexpr (std::is_same_v<T, std::string>) {
                strncpy(data.valueString, arg.c_str(), 99);
                dataType = 3;
            } // ... baaki types ke liye bhi aise hi
        }, val);
    }

    void getValue(ParamValue& outVal) const {
        switch(dataType) {
        case 0:
            outVal = data.valueInt;
            break;
        case 1:
            outVal = data.valueFloat;
            break;
        case 2:
            outVal = data.valueDouble;
            break;
        case 3:
            outVal = std::string(data.valueString);
            break;
        case 4:
            outVal = data.valueBool;
            break;
        default:
            // Unknown type, handle error
            break;
        }
    }
};

// 3D vector structure for position, rotation, scale, etc.
struct vector3 {
private:
    float x;    // X-axis value
    float y;    // Y-axis value
    float z;    // Z-axis value
public:
    void set(float newX, float newY, float newZ) {
        x = newX;
        y = newY;
        z = newZ;
    }

    void get(float& outX, float& outY, float& outZ) const {
        outX = x;
        outY = y;
        outZ = z;
    }

    float getX() const {
        return x;
    }

    float getY() const {
        return y;
    }

    float getZ() const {
        return z;
    }

    void setX(float newX) {
        x = newX;
    }

    void setY(float newY) {
        y = newY;
    }

    void setZ(float newZ) {
        z = newZ;
    }
};

// Geographic coordinate data
struct geocord {
private:
    double latitude;    // Latitude in degrees
    double longitude;   // Longitude in degrees
    double altitude;    // Altitude above sea level
    double heading;     // Direction heading in degrees
public:
    void set(double lat, double lon, double alt, double head) {
        latitude = lat;
        longitude = lon;
        altitude = alt;
        heading = head;
    }

    void get(double& outLat, double& outLon, double& outAlt, double& outHead) const {
        outLat = latitude;
        outLon = longitude;
        outAlt = altitude;
        outHead = heading;
    }

    double getLatitude() const {
        return latitude;
    }
    double getLongitude() const {
        return longitude;
    }
    double getAltitude() const {
        return altitude;
    }
    double getHeading() const {
        return heading;
    }
};

// Transform data combining geo and local space
struct transform {
    geocord geoCords;   // Global geographic coordinates
    vector3 position;  // Local position
    vector3 rotation;  // Rotation (Euler angles)
    vector3 scale;     // Scale factor
};

// Target tracking data
struct target {
    char targetID[30];          // Unique target identifier
    double latitude;           // Target latitude
    double longitude;          // Target longitude
    double altitude;           // Target altitude
    double speed;              // Target speed
    vector3 relativePosition;  // Position relative to sensor/entity
    float distance;             // Distance to target
    float bearing;              // Horizontal angle to target
    float elevation;            // Vertical angle to target
};

// Waypoint definition for trajectories
struct waypoint {
private:
    char waypointID[30];    // Waypoint identifier
    float speed;            // Desired speed at waypoint
public:
    geocord geoCords;       // Geographic position
    vector3 position;       // Local position

    float getspeed() const {
        return speed;
    }
    void setspeed(float newSpeed) {
        speed = newSpeed;
    }
    void setID(const char* newID) {
        strncpy(waypointID, newID, sizeof(waypointID) - 1);
        waypointID[sizeof(waypointID) - 1] = '\0'; // Null-terminate to avoid overflow
    }
    void getID(char* outID) const {
        strncpy(outID, waypointID, sizeof(waypointID));
    }
};

// Communication message structure
struct message {
    char messageID[30];     // Message identifier
    char content[200];      // Message payload
    double timestamp;       // Time of message creation
};

// Trajectory containing multiple waypoints

struct Trajectory {
private:
    waypoint waypoints[100]; // List of waypoints
    int waypointCount;      // Active waypoint count

public:
    void setWaypointCount(int count) {
        if(count >=0 && count <=100) {
            waypointCount = count;
        }
    }
    int getWaypointCount() const {
        return waypointCount;
    }
    waypoint getWaypoint(int index) const {
        if(index >=0 && index < waypointCount) {
            return waypoints[index];
        }
        waypoint empty;
        return empty;
    }
    void addWaypoint(const waypoint& wp,int index) {
        if(index >=0 && index <100) {
            waypoints[index] = wp;
            if(index >= waypointCount) {
                waypointCount = index + 1;
            }
        }
    }
};

// Physical and motion dynamics data
struct dynamics {
private:
    float maxSpeed;         // Maximum speed km/h
    float minSpeed;         // Minimum speed km/h
    float maxAcceleration; // Maximum acceleration (m/s²)
    float minAcceleration; // Minimum acceleration(m/s²)
    float maxDeceleration; // Maximum deceleration(m/s²)
    float minDeceleration; // Minimum deceleration(m/s²)
    float maxAltitude;      // Max operational altitude ft
    float diveRate;         // Max dive rate(ft/min)
    float climbRate;        // Max climb rate(ft/min)

    float currentSpeed; //km/h
    float currentAcceralation;//(m/s²)
    float currentHeightAboveGround;//km
    float pitch;//deg
    float roll;//deg
    float yaw;//deg
    float TrueHeading;//deg
    float Pitchrate = 0;    // Current RollRate deg/s
    float Rollrate = 0;     // Current PitchRate deg/s
    float Yawrate = 0;      // Current YawRate deg/s
    float currentTurnRate;         // Turn rate deg/s
    float horizonPitchRate; //deg/s
    float horizonRollRate; //deg/s
    float angleOfAttack; //deg
    float sideslipAngle; //deg
    float flightPathAngle; //deg
    float TrueAirSpeed; //km/h
    float GroundVelocity;//m/s
    float EastVelocity;//m/s
    float VerticalVelocity;//m/s
    float NorthVelocity;//m/s
    float DriftAngle;//deg




    parameter customParams[20]; // Custom dynamics parameters
    int paramCount;          // Number of custom parameters

public:
    vector3 groundVelocity;//m/s
    vector3 airVelocity;//m/s
    vector3 bodyAirVelocity;//m/s
    vector3 bodyGroundVelocity;//m/s
    vector3 localVelocity;//m/s
    vector3 acceleration;//(m/s²)
    vector3 bodyAcceleration;//v
    vector3 velocity;//km/h
    vector3 angularVelocity;


    void setCurrentHeightAboveGround(float v){
        currentHeightAboveGround = v;
    }
    float getCurrentHeightAboveGround(){
        return currentHeightAboveGround;
    }

    void setHorizonPitchRate(float v){
        horizonPitchRate = v;
    }
    float getHorizonPitchRate(){
        return horizonPitchRate;
    }

    void setHorizonRollRate(float v){
        horizonRollRate = v;
    }
    float getHorizonRollRate(){
        return horizonRollRate;
    }

    void setAngleOfAttack(float v){
        angleOfAttack = v;
    }
    float getAngleOfAttack(){
        return angleOfAttack;
    }

    void setSideslipAngle(float v){
        sideslipAngle = v;
    }
    float getSideslipAngle(){
        return sideslipAngle;
    }

    void setFlightPathAngle(float v){
        flightPathAngle = v;
    }
    float getFlightPathAngle(){
        return flightPathAngle;
    }

    void setGroundVelocity(float v){
        GroundVelocity = v;
    }
    float getVGroundVelocity(){
        return GroundVelocity;
    }

    void setVerticalVelocity(float v){
        VerticalVelocity = v;
    }
    float getVerticalVelocity(){
        return VerticalVelocity;
    }

    void setEastVelocity(float v){
        EastVelocity = v;
    }
    float getEastVelocity(){
        return EastVelocity;
    }

    void setNorthVelocity(float v){
        NorthVelocity = v;
    }
    float getNorthVelocity(){
        return NorthVelocity;
    }

    void setTrueAirSpeed(float v){
        TrueAirSpeed = v;
    }
    float getTrueAirSpeed(){
        return TrueAirSpeed;
    }

    void setTrueHeading(float v){
        TrueHeading = v;
    }
    float getTrueHeading(){
        return TrueHeading;
    }

    void setDriftAngle(float v){
        DriftAngle = v;
    }
    float getDriftAngle(){
        return DriftAngle;
    }

    void setPitchRate(float v){
        Pitchrate = v;
    }
    float getPitchRate(){
        return Pitchrate;
    }

    void setRollRate(float v){
        Rollrate = v;
    }
    float getRollRate(){
        return Rollrate;
    }

    void setYawRate(float v){
        Yawrate = v;
    }
    float getYawRate(){
        return Yawrate;
    }

    void setYaw(float newYaw){
        yaw = newYaw;
    }
    float getYaw(){
        return yaw;
    }

    void setRoll(float newroll){
        roll = newroll;
    }
    float getRoll(){
        return roll;
    }

    void setPitch(float newpitch){
        pitch = newpitch;
    }
    float getPitch(){
        return pitch;
    }

    void setSpeed(float newSpeed) {
        currentSpeed = newSpeed;
    }
    double getSpeed() const {
        return currentSpeed;
    }

    void setAcceleration(float newAcc) {
        currentAcceralation = newAcc;
    }
    double getAcceleration() const {
        return currentAcceralation;
    }

    void setMaxSpeed(float newMaxSpeed) {
        maxSpeed = newMaxSpeed;
    }
    double getMaxSpeed() const {
        return maxSpeed;
    }

    void setMinSpeed(float newMinSpeed) {
        minSpeed = newMinSpeed;
    }
    double getMinSpeed()  {
        return minSpeed;
    }

    void setMaxAcceleration(float newMaxAcc) {
        maxAcceleration = newMaxAcc;
    }
    double getMaxAcceleration() {
        return maxAcceleration;
    }
    void setMinAcceleration(float newMinAcc) {
        minAcceleration = newMinAcc;
    }
    double getMinAcceleration() const {
        return minAcceleration;
    }
    void setMaxDeceleration(float newMaxDec) {
        maxDeceleration = newMaxDec;
    }
    double getMaxDeceleration() const {
        return  maxDeceleration;
    }
    void setMinDeceleration(float newMinDec) {
        minDeceleration = newMinDec;
    }
    double getMinDeceleration() const {
        return minDeceleration;
    }
    void setDiveRate(float newDiveRate) {
        diveRate = newDiveRate;
    }
    double getDiveRate() const {
        return  diveRate;
    }
    void setClimbRate(float newClimbRate) {
        climbRate = newClimbRate;
    }
    double getClimbRate() const {
        return climbRate;
    }
    void setTurnRate(float newTurnRate) {
        currentTurnRate = newTurnRate;
    }
    double getTurnRate() const {
        return  currentTurnRate;
    }

    void setMaxAltitude(float newMaxAlt) {
        maxAltitude = newMaxAlt;
    }

    double getMaxAltitude() const {
        return maxAltitude;
    }

    void setCustomParameter(const parameter& param) {
        for(int i=0;i<20;i++) {
            char existingName[50];
            customParams[i].getName(existingName);
            char newName[50];
            param.getName(newName);
            if(strcmp(existingName, newName) == 0 || strlen(existingName) == 0) {
                customParams[i] = param;
                break;
            }
        }
    }

    void updateCustomParameter(int index,const parameter& param) {
        if(index >=0 && index <20) {
            customParams[index] = param;
        }
    }

    void getCustomParameter(std::string name, parameter& outParam) const {
        for(int i=0;i<20;i++) {
            char existingName[50];
            customParams[i].getName(existingName);
            if(strcmp(existingName, name.c_str()) == 0) {
                outParam = customParams[i];
                break;
            }
        }
    }

    void removeCustomParameter(std::string name) {
        for(int i=0;i<20;i++) {
            char existingName[50];
            customParams[i].getName(existingName);
            if(strcmp(existingName, name.c_str()) == 0) {
                // Reset to default
                customParams[i] = parameter();
                break;
            }
        }
    }

    void clearCustomParameters() {
        for(int i=0;i<20;i++) {
            customParams[i] = parameter();
        }
    }

    void getCustomParameterCount(int& outCount) const {
        outCount = 0;
        for(int i=0;i<20;i++) {
            char existingName[50];
            customParams[i].getName(existingName);
            if(strlen(existingName) > 0) {
                outCount++;
            }
        }
    }

    void listCustomParameterNames(std::string names[], int& outCount) const {
        outCount = 0;
        for(int i=0;i<20;i++) {
            char existingName[50];
            customParams[i].getName(existingName);
            if(strlen(existingName) > 0) {
                names[outCount] = std::string(existingName);
                outCount++;
            }
        }
    }

    void setParamCount(int count) {
        paramCount = count;
    }
    int getParamCount() const {
        return paramCount;
    }
};

// Sensor system data
struct sensor {
    char sensorID[30];      // Sensor identifier
    int sensorType;         // 0=radar, 1=lidar, 2=camera, 3=infrared
    ///Emmison
    float power = 100;//kw
    float frequency = 8.00;//ghz

    //Envolope
    float minAzimuth = 0;//deg
    float maxAzimuth = 60;//deg
    float minElevation = 0;//deg
    float maxElevation = 60;//deg

    //Scanning
    float rate = 1;
    float hits = 2;

    //Sensor Antenna
    float AntennaGain = 1;
    float AntennaBandwidth = 1;
    float beamWidth = 1;
    int scanType = 0;
    int scanTime1 = 0;
    int scanTime2 = 0;
    float peakSideLobLevel = 1;
    float avgSideLobLevel = 1;

    //Sensor Pulse
    float pulseWidth = 0;

    transform sensorTransform;  // Sensor transform data

    target trackedTargets[20];  // Currently tracked targets
    int trackedTargetCount;     // Number of tracked targets

    parameter customParams[20]; // Custom sensor parameters
};

// Radio communication system
struct radioData {
    char radioID[30];       // Radio identifier
    double distance_m = 0.0;
    double radius_m = 0.0;
    double azimuth_deg = 0.0;

    double scanner_max_range_m = 0.0;
    double scanner_frequency_hz = 0.0;

    double path_loss_db = 0.0;
    double rx_power_dbm = 0.0;
    double noise_floor_dbm = 0.0;
    double snr_db = 0.0;
    double rain_attenuation_db = 0.0;
    double wind_attenuation_db_per_km = 0.0;
    double los_horizon_distance_m = 0.0;
    double polarization_loss_db = 0.0;
    double required_snr_threshold_db = 0.0;

    bool network_match = false;
    bool frequency_match = false;
    bool range_ok = false;
    bool beam_ok = false;
    bool sensitivity_ok = false;
    bool squelch_ok = false;
    bool snr_ok = false;

    // New: transmitter-only scan result can return a footprint record
    bool is_footprint = false;
    double heading_deg = 0.0;
    double beamwidth_deg = 0.0;

    message messages[50];   // Message buffer
    int messageCount;       // Number of stored messages
    void setRadioID(const char* value)
    {
        if (!value)
        {
            radioID[0] = '\0';
            return;
        }

        std::strncpy(radioID, value, sizeof(radioID) - 1);
        radioID[sizeof(radioID) - 1] = '\0';
    }
    // parameter customParams[20]; // Custom radio parameters
};

// AIS Data Structure
struct aisData{
    // --- Static Information (Every 6 mins) ---
    uint32_t mmsi;                // Unique ID
    uint32_t imo_number;          // Official IMO ID
    char name[21];                // Vessel Name (Max 20 chars)
    char call_sign[8];            // Radio Call Sign
    char destination[21];         // Destination (Max 20 chars)

    struct Dimensions {
        uint16_t length;          // Meters
        uint16_t beam;            // Width in meters
        float draught;            // Current depth in water
    } dims;

    ShipType ship_type;

    // --- Dynamic Information (Every 2-10 seconds) ---
    struct Position {
        double latitude;          // Decimal degrees
        double longitude;         // Decimal degrees
        uint32_t timestamp;       // UTC Unix Timestamp
    } pos;

    float sog;                    // Speed Over Ground (Knots)
    float cog;                    // Course Over Ground (Degrees)
    uint16_t heading;             // True Heading (0-359)
    int8_t rot;                   // Rate of Turn (deg/min)

    NavStatus nav_status;

    // --- Voyage Related ---
    struct ETA {
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
    } eta;

    // --- Metadata / System Level ---
    uint8_t repeat_indicator;     // AIS Message repeat count
    uint8_t ais_channel;          // 0 for A, 1 for B
    bool is_verified;

    void setName(const char* nm) {
        strncpy(name, nm, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0'; // Null-terminate to avoid overflow
    }

    void setCallSign(const char* value) {
        std::strncpy(call_sign, value, sizeof(call_sign) - 1);
        call_sign[sizeof(call_sign) - 1] = '\0';
    }

    void setDestination(const char* value) {
        std::strncpy(destination, value, sizeof(destination) - 1);
        destination[sizeof(destination) - 1] = '\0';
    }
};

// ADSB Data Structure
struct adsbData {

    char messageType[16]{};
    char transmissiontype[16]{};
    char session_id[32]{};
    char aircraft_id[32]{};
    char hex_ident[16]{};
    char fligh_Id[32]{};

    char generated_date[16]{};
    char generated_time[16]{};
    char logged_date[16]{};
    char logged_time[16]{};

    char call_sign[32]{};

    int32_t altitude = 0;
    int32_t ground_speed = 0;
    int32_t track = 0;

    float lat = 0.0f;
    float lon = 0.0f;

    int32_t climb_rate = 0;

    char squawk[8]{};

    bool alert = false;
    bool emergency = false;
    bool spi = false;
    bool is_on_ground = false;

    void setMessageType(const char* value) {
        std::strncpy(messageType, value, sizeof(messageType) - 1);
        messageType[sizeof(messageType) - 1] = '\0';
    }

    void setTransmissionType(const char* value) {
        std::strncpy(transmissiontype, value, sizeof(transmissiontype) - 1);
        transmissiontype[sizeof(transmissiontype) - 1] = '\0';
    }

    void setSessionId(const char* value) {
        std::strncpy(session_id, value, sizeof(session_id) - 1);
        session_id[sizeof(session_id) - 1] = '\0';
    }

    void setAircraftId(const char* value) {
        std::strncpy(aircraft_id, value, sizeof(aircraft_id) - 1);
        aircraft_id[sizeof(aircraft_id) - 1] = '\0';
    }

    void setHexIdent(const char* value) {
        std::strncpy(hex_ident, value, sizeof(hex_ident) - 1);
        hex_ident[sizeof(hex_ident) - 1] = '\0';
    }

    void setFlightId(const char* value) {
        std::strncpy(fligh_Id, value, sizeof(fligh_Id) - 1);
        fligh_Id[sizeof(fligh_Id) - 1] = '\0';
    }

    void setGeneratedDate(const char* value) {
        std::strncpy(generated_date, value, sizeof(generated_date) - 1);
        generated_date[sizeof(generated_date) - 1] = '\0';
    }

    void setGeneratedTime(const char* value) {
        std::strncpy(generated_time, value, sizeof(generated_time) - 1);
        generated_time[sizeof(generated_time) - 1] = '\0';
    }

    void setLoggedDate(const char* value) {
        std::strncpy(logged_date, value, sizeof(logged_date) - 1);
        logged_date[sizeof(logged_date) - 1] = '\0';
    }

    void setLoggedTime(const char* value) {
        std::strncpy(logged_time, value, sizeof(logged_time) - 1);
        logged_time[sizeof(logged_time) - 1] = '\0';
    }

    void setCallSign(const char* value) {
        std::strncpy(call_sign, value, sizeof(call_sign) - 1);
        call_sign[sizeof(call_sign) - 1] = '\0';
    }

    void setSquawk(const char* value) {
        std::strncpy(squawk, value, sizeof(squawk) - 1);
        squawk[sizeof(squawk) - 1] = '\0';
    }
};

// Identification Friend or Foe (IFF) system
struct iff {
    char code[20];          // IFF code
    int status;             // 0=unknown, 1=friendly, 2=hostile

    parameter customParams[20]; // Custom IFF parameters
};

// Bitmap / visual representation data
struct bitmap {
    int width;              // Bitmap width
    int height;             // Bitmap height
    char sensorID[100];     // Associated sensor ID
};

// Collision shape definition
struct collider {
    char colliderID[30];    // Collider identifier
    int colliderType;       // 0=sphere, 1=box, 2=capsule

    float radius;           // Radius (for sphere/capsule)
    vector3 size;           // Size (for box)
    vector3 positionOffset; // Offset from entity origin
};

// Radar / visual cross-section data
struct crossSection {
    float length;           // Length of entity
    float width;            // Width of entity
    float height;           // Height of entity
};

// Main entity structure
struct Entity {
private:
    char ID[30];                    // Unique entity ID
    char name[50];                  // Entity name
public:
    transform transformData;        // Transform data
    collider colliderData;          // Collision data
    dynamics dynamicsData;          // Motion dynamics
    crossSection crossSectionData;  // Physical dimensions
    Trajectory trajectoryData;      // Movement trajectory
    bitmap entityBitmap;            // Visual/bitmap data

    //sensor sensors[5];              // Sensors attached
    radioData radios[100];          // Radios attached
    aisData ais[100];
    adsbData adsb[100];
    iff iffs[5];                    // IFF systems attached

    int iffCount;                   // Active IFF count
    int sensorCount;                // Active sensor count
    int radioCount;                 // Active radio count
    int aisCount;
    int adsbCount;

    void setID(const char* newID) {
        strncpy(ID, newID, sizeof(ID) - 1);
        ID[sizeof(ID) - 1] = '\0'; // Null-terminate to avoid overflow
    }

    void setName(const char* newName) {
        strncpy(name, newName, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0'; // Null-terminate to avoid overflow
    }

    void getID(char* outID) const {
        strncpy(outID, ID, sizeof(ID));
    }

    void getName(char* outName) const {
        strncpy(outName, name, sizeof(name));
    }
};

// Shared world data containing all entities
struct SharedData {
    Entity entities[1000];   // Global entity list
    int entityCount;        // Active entity count

};
}
