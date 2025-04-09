#include "stations.h"
#include <string.h>

// Available destinations
static const Destination destinations[] = {
    {1, "Polgahawela"},
    {2, "Alawwa"},
    {3, "Ambepussa"},
    {4, "Botale"},
    {5, "Wellawatte"},
    {6, "Mirigama"},
    {7, "Ganegoda"},
    {8, "Veyangoda"},
    {9, "Heendeniya"},
    {10, "Gampaha"},
    {11, "Ganemulla"},
    {12, "Ragama"},
    {13, "Enderamulla"},
    {14, "Kelaniya"},
    {15, "Dematagoda"},
    {16, "Maradana"},
    {17, "Colombo Fort"}};

// Available classes
static const TrainClass train_classes[] = {
    {1, "First class"},
    {2, "Second class"},
    {3, "Third class"}};

// Number of destinations and classes
#define NUM_DESTINATIONS (sizeof(destinations) / sizeof(destinations[0]))
#define NUM_CLASSES (sizeof(train_classes) / sizeof(train_classes[0]))

// Function to get the number of destinations
int get_number_of_destinations(void)
{
    return NUM_DESTINATIONS;
}

// Function to get the number of train classes
int get_number_of_train_classes(void)
{
    return NUM_CLASSES;
}

// Function to get the destinations array
const Destination *get_destinations(void)
{
    return destinations;
}

// Function to get the train classes array
const TrainClass *get_train_classes(void)
{
    return train_classes;
}

// Find destination by ID
const char *get_destination_name(int id)
{
    for (int i = 0; i < NUM_DESTINATIONS; i++)
    {
        if (destinations[i].id == id)
        {
            return destinations[i].name;
        }
    }
    return "Unknown";
}

// Find class by ID
const char *get_class_name(int id)
{
    for (int i = 0; i < NUM_CLASSES; i++)
    {
        if (train_classes[i].id == id)
        {
            return train_classes[i].name;
        }
    }
    return "Unknown";
}