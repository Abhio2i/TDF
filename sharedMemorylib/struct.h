#include <cstring>
#include <variant>
#include <string>

namespace SharedStructs {
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
    float maxSpeed;         // Maximum speed
    float minSpeed;         // Minimum speed
    float maxAcceleration; // Maximum acceleration
    float minAcceleration; // Minimum acceleration
    float maxDeceleration; // Maximum deceleration
    float minDeceleration; // Minimum deceleration
    float maxAltitude;      // Max operational altitude
    float diveRate;         // Max dive rate
    float climbRate;        // Max climb rate
    float turnRate;         // Turn rate
    float acceleration;     // Current accelerationoid
    float speed;            // Current speed
    float pitch = 0;        // Current pitch
    float roll = 0;         // Current Roll
    float yaw = 0;          // Current Yaw
    float Rollrate = 0;     // Current PitchRate
    float Pitchrate = 0;    // Current RollRate
    float Yawrate = 0;      // Current YawRate
    float DriftAngle = 0;   // Current DriftAngle
    float TrueHeading = 0;  // Current TrueHeading
    float TrueAirSpeed = 0; // Current TrueAirSpeed
    float NorthVelocity = 0;// Current NorthVelocity
    float EastVelocity = 0; // Current EastVelocity
    float VerticalVelocity = 0;// Current VerticleVelocity
    float GroundVelocity = 0;// Current GroundVelocity
    parameter customParams[20]; // Custom dynamics parameters
    int paramCount;          // Number of custom parameters

    public:
    vector3 velocity;
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
        speed = newSpeed;
    }
    double getSpeed() const {
        return speed;
    }

    void setAcceleration(float newAcc) {
        acceleration = newAcc;
    }
    double getAcceleration() const {
        return acceleration;
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
        turnRate = newTurnRate;
    }
    double getTurnRate() const {
        return  turnRate;
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
struct radio {
    char radioID[30];       // Radio identifier
    float frequency;        // Communication frequency
    float channelBandwidth; // Channel bandwidth
    float power;            // Transmission power
    float range;            // Communication range

    message messages[50];   // Message buffer
    int messageCount;       // Number of stored messages

    parameter customParams[20]; // Custom radio parameters
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

        sensor sensors[5];              // Sensors attached
        radio radios[5];                // Radios attached
        iff iffs[5];                    // IFF systems attached

        int iffCount;                   // Active IFF count
        int sensorCount;                // Active sensor count
        int radioCount;                 // Active radio count

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