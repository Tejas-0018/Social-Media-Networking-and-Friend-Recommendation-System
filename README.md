# Social-Media-Networking-and-Friend-Recommendation-System


Project Overview

This project is a C-based simulation of a social media network featuring a command-line interface and a robust friend recommendation system. The application models the network utilizing a graph data structure, where individual users are represented as vertices and their mutual friendships are represented as undirected edges. By focusing on backend logic and data structure management, the system successfully implements core social platform functionalities including dynamic user registration, profile management, and bidirectional connection handling.


System Architecture and Data Structures

The network's architecture is built upon a highly efficient adjacency list implemented as a dynamic array of pointers. Each index in this primary array points to a linked list of node structures representing a specific user's connections. To maintain user state and profile information without modifying the core graph, the system utilizes two parallel dynamic arrays. A user profile array allows for constant-time lookup of individual details such as name and city, while a boolean registration array acts as a status map to track active and deleted members.


Core Algorithms and Scalability

To ensure the network can grow beyond any initial fixed capacity, the system employs a dynamic resizing algorithm utilizing memory allocation functions to efficiently preserve existing user data while expanding the arrays. The deletion algorithm safely removes users by first applying a soft deletion via the boolean tracking array, followed by a thorough traversal to sever all incoming and outgoing edge connections to prevent dangling pointers. The centerpiece of the application is the "Friends of Friends" recommendation engine. This algorithm identifies second-degree connections by traversing the adjacency lists of a user's direct friends, applying strict filtering logic to exclude existing friends and prevent duplicate suggestions.


Time Complexity and Future Enhancements

The strategic use of adjacency lists and parallel tracking arrays ensures highly efficient time complexities for most operations. For example, registering a user operates in amortized constant time relative to the insertion, while the recommendation algorithm operates efficiently depending on the network's capacity and the maximum degree of a node. Future enhancements for this system include implementing data persistence to save the network state to a file, introducing traversal algorithms like Breadth-First Search for calculating degrees of separation, and developing more advanced recommendation ranking systems.
