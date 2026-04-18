#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// User profile structure to hold detailed information about a user.
typedef struct UserProfile
{
    int userID;
    char name[50];
    int age;
    char email[100];
    char city[50];
} user;

// Adjacency list node represents a connection (edge) in the graph.
typedef struct Node
{
    int vertex;
    struct Node *next;
} Node;

// Pointers for our dynamic arrays
bool *registered;
user *profiles;
Node **adjlist; // Pointer to an array of Node pointers.

int user_count = 0; // The number of users currently registered.
int capacity = 0;   // The current allocated size of our arrays.

// initiallizing the network with a given capacity
void initNetwork(int start_capacity)
{
    capacity = start_capacity;

    // Allocate initial memory from the heap.
    registered = (bool *)malloc(capacity * sizeof(bool));
    profiles = (user *)malloc(capacity * sizeof(user));
    adjlist = (Node **)malloc(capacity * sizeof(Node *));

    if (registered == NULL || profiles == NULL || adjlist == NULL)
    {
        printf("FATAL: Initial memory allocation failed. Exiting.\n");
        exit(1);
    }

    // Initialize all entries to default values.
    for (int i = 0; i < capacity; i++)
    {
        registered[i] = false;
        adjlist[i] = NULL;
    }
    printf("Network initialized with capacity for %d users.\n", capacity);
}

// resizing the network when full
void resizeNetwork()
{
    int new_capacity = capacity * 2;
    printf("Network is full. Resizing capacity from %d to %d...\n", capacity, new_capacity);

    // Use realloc to efficiently resize the arrays, preserving existing data.
    registered = (bool *)realloc(registered, new_capacity * sizeof(bool));
    profiles = (user *)realloc(profiles, new_capacity * sizeof(user));
    adjlist = (Node **)realloc(adjlist, new_capacity * sizeof(Node *));

    if (registered == NULL || profiles == NULL || adjlist == NULL)
    {
        printf("FATAL: Memory reallocation failed. Exiting.\n");
        exit(1);
    }

    // Initialize the new portion of the arrays.
    for (int i = capacity; i < new_capacity; i++)
    {
        registered[i] = false;
        adjlist[i] = NULL;
    }

    capacity = new_capacity; // Update the global capacity variable.
}

// funct6ion to find user id by name
int findUserIdByName(const char *name)
{

    for (int i = 0; i < user_count; i++)
    {
        if (registered[i] && strcmp(profiles[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

// Frees all dynamically allocated memory
void cleanupNetwork()
{

    for (int i = 0; i < user_count; i++)
    {
        if (registered[i])
        {
            Node *temp = adjlist[i];
            while (temp != NULL)
            {
                Node *tofree = temp;
                temp = temp->next;
                free(tofree);
            }
        }
    }

    free(registered);
    free(profiles);
    free(adjlist);
}

void addEdge(int s, int d)
{
    Node *dest = (Node *)malloc(sizeof(Node));
    dest->vertex = d;
    dest->next = adjlist[s];
    adjlist[s] = dest;
}

void addConnection(int s, int d)
{
    if (!registered[s] || !registered[d] || s == d)
        return;

    // Check if a connection already exists to prevent duplicates.
    Node *temp = adjlist[s];
    while (temp != NULL)
    {
        if (temp->vertex == d)
        {
            printf("Info: Connection between '%s' and '%s' already exists.\n", profiles[s].name, profiles[d].name);
            return;
        }
        temp = temp->next;
    }

    addEdge(s, d);
    addEdge(d, s); // Add the reverse edge for an undirected graph.
    printf("Success: Connection between '%s' and '%s' added.\n", profiles[s].name, profiles[d].name);
}

static void removeEdge(int s, int d)
{
    Node *temp = adjlist[s];
    Node *prev = NULL;
    while (temp != NULL && temp->vertex != d)
    {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL)
        return; // Edge not found.
    if (prev == NULL)
    {
        adjlist[s] = temp->next; // The node to remove is the head.
    }
    else
    {
        prev->next = temp->next;
    }
    free(temp);
}

void deleteConnection(int s, int d)
{
    if (!registered[s] || !registered[d])
        return;
    removeEdge(s, d);
    removeEdge(d, s);
    printf("Success: Connection between '%s' and '%s' deleted.\n", profiles[s].name, profiles[d].name);
}

void deletenode(int n)
{
    if (n >= user_count || !registered[n])
        return;

    // Remove all incoming edges to this node from other users.
    for (int i = 0; i < user_count; i++)
    {
        if (registered[i] && i != n)
        {
            removeEdge(i, n);
        }
    }

    // Free the entire adjacency list of the deleted node.
    Node *temp = adjlist[n];
    while (temp != NULL)
    {
        Node *tofree = temp;
        temp = temp->next;
        free(tofree);
    }
    adjlist[n] = NULL;

    registered[n] = false;

    printf("Success: User '%s' has been deleted.\n", profiles[n].name);
}

// Displays the entire social network graph, showing users and their connections.

void display()
{
    printf("\n--- Social Network Graph ---\n");
    int active_count = 0;
    for (int i = 0; i < user_count; i++)
    {
        if (registered[i])
        {
            active_count++;
            Node *temp = adjlist[i];
            printf("User '%s' (%d) is connected to: ", profiles[i].name, i);
            while (temp != NULL)
            {
                printf("'%s' (%d) -> ", profiles[temp->vertex].name, temp->vertex);
                temp = temp->next;
            }
            printf("NULL\n");
        }
    }
    if (active_count == 0)
    {
        printf("The network is empty.\n");
    }
    printf("--------------------------\n\n");
}

// Displays the detailed profile of a single user.

void viewProfile(int userId)
{
    if (userId >= user_count || !registered[userId])
        return;

    printf("\n--- Profile for User '%s' ---\n", profiles[userId].name);
    printf("ID:   %d\n", profiles[userId].userID);
    printf("Age:  %d\n", profiles[userId].age);
    printf("Email: %s\n", profiles[userId].email);
    printf("City: %s\n", profiles[userId].city);
    printf("-----------------------------\n");
}

// Recommends new friends to a user based on the "Friends of Friends" algorithm.

void recommendation(int user_ID)
{
    if (user_ID >= user_count || !registered[user_ID])
        return;

    int friends_recommended = 0;
    // Use calloc to initialize boolean arrays to all false.
    bool *isfriend = (bool *)calloc(capacity, sizeof(bool));
    bool *recommended = (bool *)calloc(capacity, sizeof(bool));

    if (isfriend == NULL || recommended == NULL)
    {
        printf("Error: Could not allocate memory for recommendation tracking.\n");
        return;
    }

    printf("Friend recommendations for User '%s':\n", profiles[user_ID].name);

    isfriend[user_ID] = true; // Exclude user from their own recommendations.
    Node *temp = adjlist[user_ID];
    while (temp != NULL)
    {
        isfriend[temp->vertex] = true;
        temp = temp->next;
    }

    Node *L1_friends = adjlist[user_ID];
    while (L1_friends != NULL)
    {
        int friend_ID = L1_friends->vertex;

        Node *L2_friends = adjlist[friend_ID];
        while (L2_friends != NULL)
        {
            int potential_friend_id = L2_friends->vertex;

            if (!isfriend[potential_friend_id] && !recommended[potential_friend_id])
            {
                printf(" -> '%s' (friend of '%s')\n", profiles[potential_friend_id].name, profiles[friend_ID].name);
                recommended[potential_friend_id] = true;
                friends_recommended++;
            }
            L2_friends = L2_friends->next;
        }
        L1_friends = L1_friends->next;
    }

    if (friends_recommended == 0)
    {
        printf("No new friend recommendations available.\n");
    }

    free(isfriend);
    free(recommended);
}

// register user function
void registerUserByName()
{
    // Check if the network needs to be resized before adding a new user.
    if (user_count == capacity)
    {
        resizeNetwork();
    }

    // The ID for the new user is simply the current total user count.
    int newId = user_count;

    char newName[50];
    printf("Enter name for the new user: ");
    scanf(" %49[^\n]", newName);

    if (findUserIdByName(newName) != -1)
    {
        printf("Error: A user with the name '%s' already exists.\n", newName);
        return;
    }

    registered[newId] = true;
    profiles[newId].userID = newId;
    strcpy(profiles[newId].name, newName);

    printf("Enter age for %s: ", newName);
    scanf("%d", &profiles[newId].age);
    printf("Enter email for %s: ", newName);
    scanf(" %99[^\n]", profiles[newId].email);
    printf("Enter city for %s: ", newName);
    scanf(" %49[^\n]", profiles[newId].city);

    user_count++;

    printf("Success: User '%s' registered with ID %d.\n", newName, newId);
}

void addConnectionByName()
{
    char name1[50], name2[50];
    printf("Enter the name of the first user: ");
    scanf(" %49[^\n]", name1);
    printf("Enter the name of the second user: ");
    scanf(" %49[^\n]", name2);

    int id1 = findUserIdByName(name1);
    int id2 = findUserIdByName(name2);

    if (id1 == -1)
    {
        printf("Error: User '%s' not found.\n", name1);
        return;
    }
    if (id2 == -1)
    {
        printf("Error: User '%s' not found.\n", name2);
        return;
    }

    addConnection(id1, id2);
}

void deleteConnectionByName()
{
    char name1[50], name2[50];
    printf("Enter the name of the first user: ");
    scanf(" %49[^\n]", name1);
    printf("Enter the name of the second user: ");
    scanf(" %49[^\n]", name2);

    int id1 = findUserIdByName(name1);
    int id2 = findUserIdByName(name2);

    if (id1 == -1)
    {
        printf("Error: User '%s' not found.\n", name1);
        return;
    }
    if (id2 == -1)
    {
        printf("Error: User '%s' not found.\n", name2);
        return;
    }

    deleteConnection(id1, id2);
}

void deleteNodeByName()
{
    char name[50];
    printf("Enter the name of the user to delete: ");
    scanf(" %49[^\n]", name);
    int id = findUserIdByName(name);
    if (id == -1)
    {
        printf("Error: User '%s' not found.\n", name);
        return;
    }
    deletenode(id);
}

void viewProfileByName()
{
    char name[50];
    printf("Enter the name of the user to view: ");
    scanf(" %49[^\n]", name);
    int id = findUserIdByName(name);
    if (id == -1)
    {
        printf("Error: User '%s' not found.\n", name);
        return;
    }
    viewProfile(id);
}

void recommendationByName()
{
    char name[50];
    printf("Enter your name to get recommendations: ");
    scanf(" %49[^\n]", name);
    int id = findUserIdByName(name);
    if (id == -1)
    {
        printf("Error: User '%s' not found.\n", name);
        return;
    }
    recommendation(id);
}

int main()
{
    int choice;

    initNetwork(10);

    while (1)
    {
        printf("\n===== Social Network Menu =====\n");
        printf("1. Register a new user\n");
        printf("2. Add a connection\n");
        printf("3. Delete a connection\n");
        printf("4. Delete a user\n");
        printf("5. Display the network graph\n");
        printf("6. Get friend recommendations\n");
        printf("7. View a user profile\n");
        printf("0. Exit\n");
        printf("=============================\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            registerUserByName();
            break;
        case 2:
            addConnectionByName();
            break;
        case 3:
            deleteConnectionByName();
            break;
        case 4:
            deleteNodeByName();
            break;
        case 5:
            display();
            break;
        case 6:
            recommendationByName();
            break;
        case 7:
            viewProfileByName();
            break;
        case 0:
            printf("Cleaning up memory and exiting. Goodbye!\n");
            cleanupNetwork(); // Free all allocated memory before exiting.
            exit(0);
        default:
            printf("Invalid choice! Please try again.\n");
            break;
        }
    }
    return 0;
}