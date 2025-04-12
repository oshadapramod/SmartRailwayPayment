#ifndef STATIONS_H
#define STATIONS_H

// Define destination structure
typedef struct
{
    int id;
    char name[20];
} Destination;

// Define class structure
typedef struct
{
    int id;
    char name[15];
} TrainClass;

// Function to get the number of destinations
int get_number_of_destinations(void);

// Function to get the number of train classes
int get_number_of_train_classes(void);

// Getter functions to access destinations and classes
const Destination* get_destinations(void);
const TrainClass* get_train_classes(void);

// Helper functions to get names by ID
const char* get_destination_name(int id);
const char* get_class_name(int id);

#endif // STATIONS_H