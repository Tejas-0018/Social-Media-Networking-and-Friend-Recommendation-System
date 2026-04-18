#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// ===================================================================================
// ==                                                                               ==
// ==                      BACKEND: SOCIAL NETWORK ENGINE                           ==
// ==                                                                               ==
// ===================================================================================

// A color_index is added to the UserProfile for visualization purposes.
typedef struct UserProfile {
    int userID;
    char name[50];
    int age;
    char email[100];
    char city[50];
    int color_index; // To assign a unique color to each user node.
} user;

// Adjacency list node represents a connection (edge) in the graph.
typedef struct Node {
    int vertex;
    struct Node *next;
} Node;

// ---- DYNAMIC GLOBAL VARIABLES ----
bool *registered;
user *profiles;
Node **adjlist;
int user_count = 0;
int capacity = 0;

// ---- ENHANCED RECOMMENDATION DATA STRUCTURE ----
// Now stores the names of mutual friends for a more detailed UI.
typedef struct RecNode {
    int userId;
    int mutualFriendCount;
    char mutualFriendNames[150]; // Buffer to store names like "Alice, Bob..."
} RecNode;

// ---- MEMORY MANAGEMENT & HELPER FUNCTIONS ----

void initNetwork(int start_capacity) {
    capacity = start_capacity;
    registered = (bool*)calloc(capacity, sizeof(bool));
    profiles = (user*)malloc(capacity * sizeof(user));
    adjlist = (Node**)calloc(capacity, sizeof(Node*));
    if (registered == NULL || profiles == NULL || adjlist == NULL) {
        fprintf(stderr, "FATAL: Initial memory allocation failed. Exiting.\n");
        exit(1);
    }
}

void resizeNetwork() {
    int new_capacity = (capacity == 0) ? 10 : capacity * 2;
    printf("Network is full. Resizing capacity from %d to %d...\n", capacity, new_capacity);
    registered = (bool*)realloc(registered, new_capacity * sizeof(bool));
    profiles = (user*)realloc(profiles, new_capacity * sizeof(user));
    adjlist = (Node**)realloc(adjlist, new_capacity * sizeof(Node*));
    if (registered == NULL || profiles == NULL || adjlist == NULL) {
        fprintf(stderr, "FATAL: Memory reallocation failed. Exiting.\n");
        exit(1);
    }
    // Initialize the new portion of the arrays.
    for (int i = capacity; i < new_capacity; i++) {
        registered[i] = false;
        adjlist[i] = NULL;
    }
    capacity = new_capacity;
}

void cleanupNetwork() {
    for (int i = 0; i < user_count; i++) {
        if(registered[i]) {
            Node* temp = adjlist[i];
            while (temp != NULL) {
                Node* tofree = temp;
                temp = temp->next;
                free(tofree);
            }
        }
    }
    free(registered);
    free(profiles);
    free(adjlist);
}

int findUserIdByName(const char* name) {
    for (int i = 0; i < user_count; i++) {
        if (registered[i] && strcmp(profiles[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// ---- CORE GRAPH LOGIC ----

static void addEdge(int s, int d) {
    Node *dest = (Node *)malloc(sizeof(Node));
    dest->vertex = d;
    dest->next = adjlist[s];
    adjlist[s] = dest;
}

const char* addConnection(int s, int d) {
    if (!registered[s] || !registered[d] || s == d) return "Invalid users or same user.";
    Node *temp = adjlist[s];
    while (temp != NULL) {
        if (temp->vertex == d) return "Connection already exists.";
        temp = temp->next;
    }
    addEdge(s, d);
    addEdge(d, s);
    return "Connection added successfully.";
}

static void removeEdge(int s, int d) {
    Node *temp = adjlist[s];
    Node *prev = NULL;
    while (temp != NULL && temp->vertex != d) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) return;
    if (prev == NULL) {
        adjlist[s] = temp->next;
    } else {
        prev->next = temp->next;
    }
    free(temp);
}

void deleteConnection(int s, int d) {
    if (!registered[s] || !registered[d]) return;
    removeEdge(s, d);
    removeEdge(d, s);
}

void deletenode(int n) {
    if (n >= user_count || !registered[n]) return;
    for (int i = 0; i < user_count; i++) {
        if (registered[i] && i != n) {
            removeEdge(i, n);
        }
    }
    Node *temp = adjlist[n];
    while (temp != NULL) {
        Node *tofree = temp;
        temp = temp->next;
        free(tofree);
    }
    adjlist[n] = NULL;
    registered[n] = false;
}

// ---- ENHANCED RECOMMENDATION SYSTEM ----

int compareRecs(const void *a, const void *b) {
    RecNode *recA = (RecNode *)a;
    RecNode *recB = (RecNode *)b;
    return recB->mutualFriendCount - recA->mutualFriendCount;
}

RecNode* get_recommendations(int user_ID, int* rec_count) {
    if (user_ID >= user_count || !registered[user_ID]) {
        *rec_count = 0;
        return NULL;
    }

    bool *isfriend = (bool*)calloc(capacity, sizeof(bool));
    bool *recommended_flags = (bool*)calloc(capacity, sizeof(bool));
    RecNode* temp_recs = (RecNode*)malloc(capacity * sizeof(RecNode));
    int current_rec_count = 0;

    if (!isfriend || !recommended_flags || !temp_recs) {
        fprintf(stderr, "Failed to allocate memory for recommendations.\n");
        free(isfriend); free(recommended_flags); free(temp_recs);
        *rec_count = 0;
        return NULL;
    }

    isfriend[user_ID] = true;
    Node *temp = adjlist[user_ID];
    while (temp != NULL) {
        isfriend[temp->vertex] = true;
        temp = temp->next;
    }

    Node *L1_friends = adjlist[user_ID];
    while (L1_friends != NULL) {
        Node *L2_friends = adjlist[L1_friends->vertex];
        while (L2_friends != NULL) {
            int potential_friend_id = L2_friends->vertex;
            if (!isfriend[potential_friend_id] && !recommended_flags[potential_friend_id]) {
                int mutual_count = 0;
                char mutual_names_buffer[150] = "";
                Node* pf_friends = adjlist[potential_friend_id];
                while (pf_friends != NULL) {
                    if (isfriend[pf_friends->vertex]) {
                        if (mutual_count < 3) {
                            if (mutual_count > 0) strcat(mutual_names_buffer, ", ");
                            strcat(mutual_names_buffer, profiles[pf_friends->vertex].name);
                        }
                        mutual_count++;
                    }
                    pf_friends = pf_friends->next;
                }

                if (mutual_count > 0) {
                    temp_recs[current_rec_count].userId = potential_friend_id;
                    temp_recs[current_rec_count].mutualFriendCount = mutual_count;
                    if(mutual_count > 3) sprintf(mutual_names_buffer + strlen(mutual_names_buffer), " and %d more...", mutual_count - 3);
                    strcpy(temp_recs[current_rec_count].mutualFriendNames, mutual_names_buffer);
                    current_rec_count++;
                }
                recommended_flags[potential_friend_id] = true;
            }
            L2_friends = L2_friends->next;
        }
        L1_friends = L1_friends->next;
    }
    
    free(isfriend);
    free(recommended_flags);
    qsort(temp_recs, current_rec_count, sizeof(RecNode), compareRecs);
    
    *rec_count = current_rec_count;
    return temp_recs;
}


// ===================================================================================
// ==                     FRONTEND: GTK GRAPHICAL USER INTERFACE                    ==
// ===================================================================================

// Global widgets and state
GtkWidget *drawing_area;
GtkWidget *status_label;
GtkWidget *main_window;
int newly_added_id = -1;

#define NUM_COLORS 8
const GdkRGBA user_colors[NUM_COLORS] = {
    {0.20, 0.59, 0.85, 1.0}, {0.95, 0.43, 0.13, 1.0}, {0.30, 0.68, 0.31, 1.0},
    {0.84, 0.15, 0.15, 1.0}, {0.58, 0.40, 0.74, 1.0}, {0.18, 0.74, 0.74, 1.0},
    {0.90, 0.29, 0.54, 1.0}, {0.50, 0.50, 0.50, 1.0}
};

static gboolean turn_off_glow(gpointer user_data) {
    newly_added_id = -1;
    gtk_widget_queue_draw(drawing_area);
    return G_SOURCE_REMOVE;
}

static gboolean draw_graph(GtkWidget *widget, cairo_t *cr, gpointer data) {
    guint width = gtk_widget_get_allocated_width(widget);
    guint height = gtk_widget_get_allocated_height(widget);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    if (user_count == 0) return FALSE;

    double center_x = width / 2.0;
    double center_y = height / 2.0;
    double radius = (width < height ? width : height) * 0.40;
    double node_radius = 25.0;

    GArray *positions = g_array_new(FALSE, FALSE, sizeof(GdkPoint));
    for (int i = 0; i < user_count; i++) {
        double angle = 2.0 * G_PI * i / user_count;
        GdkPoint p = {(int)(center_x + radius * cos(angle)), (int)(center_y + radius * sin(angle))};
        g_array_append_val(positions, p);
    }

    cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 0.5);
    cairo_set_line_width(cr, 2.0);
    for (int i = 0; i < user_count; i++) {
        if (registered[i]) {
            Node* temp = adjlist[i];
            while (temp != NULL) {
                if (i < temp->vertex) {
                    GdkPoint p1 = g_array_index(positions, GdkPoint, i);
                    GdkPoint p2 = g_array_index(positions, GdkPoint, temp->vertex);
                    cairo_move_to(cr, p1.x, p1.y);
                    cairo_line_to(cr, p2.x, p2.y);
                    cairo_stroke(cr);
                }
                temp = temp->next;
            }
        }
    }

    for (int i = 0; i < user_count; i++) {
        if (registered[i]) {
            GdkPoint p = g_array_index(positions, GdkPoint, i);
            if (i == newly_added_id) {
                cairo_set_source_rgba(cr, 1.0, 0.9, 0.2, 0.6);
                cairo_arc(cr, p.x, p.y, node_radius + 6.0, 0, 2 * G_PI);
                cairo_fill(cr);
            }
            gdk_cairo_set_source_rgba(cr, &user_colors[profiles[i].color_index]);
            cairo_arc(cr, p.x, p.y, node_radius, 0, 2 * G_PI);
            cairo_fill(cr);
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_set_font_size(cr, 12);
            cairo_text_extents_t extents;
            char display_name[20];
            strncpy(display_name, profiles[i].name, 8);
            if (strlen(profiles[i].name) > 8) {
                display_name[8] = '\0';
                strcat(display_name, "...");
            } else {
                display_name[strlen(profiles[i].name)] = '\0';
            }
            cairo_text_extents(cr, display_name, &extents);
            cairo_move_to(cr, p.x - extents.width / 2 - extents.x_bearing, p.y - extents.height / 2 - extents.y_bearing);
            cairo_show_text(cr, display_name);
        }
    }
    g_array_free(positions, TRUE);
    return FALSE;
}

// ---- CALLBACK FUNCTIONS ----

static void show_profile_dialog(const char* name) {
    int id = findUserIdByName(name);
    if (id == -1) {
        gtk_label_set_text(GTK_LABEL(status_label), "Status: User not found.");
        return;
    }
    char profile_details[512];
    sprintf(profile_details, "<b>ID:</b> %d\n<b>Age:</b> %d\n<b>Email:</b> %s\n<b>City:</b> %s",
            profiles[id].userID, profiles[id].age, profiles[id].email, profiles[id].city);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "Profile for '%s'", name);
    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog), "%s", profile_details);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_register_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *name_entry = g_object_get_data(G_OBJECT(button), "name_entry");
    GtkWidget *age_entry = g_object_get_data(G_OBJECT(button), "age_entry");
    GtkWidget *email_entry = g_object_get_data(G_OBJECT(button), "email_entry");
    GtkWidget *city_entry = g_object_get_data(G_OBJECT(button), "city_entry");
    const char* name = gtk_entry_get_text(GTK_ENTRY(name_entry));
    const char* age_str = gtk_entry_get_text(GTK_ENTRY(age_entry));
    const char* email = gtk_entry_get_text(GTK_ENTRY(email_entry));
    const char* city = gtk_entry_get_text(GTK_ENTRY(city_entry));
    if (strlen(name) == 0) { gtk_label_set_text(GTK_LABEL(status_label), "Status: Name cannot be empty."); return; }
    if (user_count == capacity) resizeNetwork();
    if (findUserIdByName(name) != -1) { gtk_label_set_text(GTK_LABEL(status_label), "Status: User with this name already exists."); return; }
    int newId = user_count;
    registered[newId] = true;
    profiles[newId].userID = newId;
    strcpy(profiles[newId].name, name);
    profiles[newId].age = atoi(age_str);
    strcpy(profiles[newId].email, email);
    strcpy(profiles[newId].city, city);
    profiles[newId].color_index = newId % NUM_COLORS;
    user_count++;
    newly_added_id = newId;
    g_timeout_add(1500, (GSourceFunc)turn_off_glow, NULL);
    char status[128];
    sprintf(status, "Status: User '%s' registered successfully.", name);
    gtk_label_set_text(GTK_LABEL(status_label), status);
    gtk_entry_set_text(GTK_ENTRY(name_entry), "");
    gtk_entry_set_text(GTK_ENTRY(age_entry), "");
    gtk_entry_set_text(GTK_ENTRY(email_entry), "");
    gtk_entry_set_text(GTK_ENTRY(city_entry), "");
    gtk_widget_queue_draw(drawing_area);
}

static void on_connect_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *name1_entry = g_object_get_data(G_OBJECT(button), "name1_entry");
    GtkWidget *name2_entry = g_object_get_data(G_OBJECT(button), "name2_entry");
    const char* name1 = gtk_entry_get_text(GTK_ENTRY(name1_entry));
    const char* name2 = gtk_entry_get_text(GTK_ENTRY(name2_entry));
    int id1 = findUserIdByName(name1);
    int id2 = findUserIdByName(name2);
    if (id1 == -1 || id2 == -1) { gtk_label_set_text(GTK_LABEL(status_label), "Status: One or both users not found."); return; }
    const char* result = addConnection(id1, id2);
    char status[128];
    sprintf(status, "Status: %s", result);
    gtk_label_set_text(GTK_LABEL(status_label), status);
    gtk_widget_queue_draw(drawing_area);
}

static void on_rec_skip_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *row_to_remove = GTK_WIDGET(user_data);
    gtk_widget_destroy(row_to_remove);
}

static void on_rec_add_friend_clicked(GtkButton *button, gpointer user_data) {
    const char* user1_name = (const char*)g_object_get_data(G_OBJECT(button), "user1_name");
    const char* user2_name = (const char*)g_object_get_data(G_OBJECT(button), "user2_name");
    int id1 = findUserIdByName(user1_name);
    int id2 = findUserIdByName(user2_name);
    if (id1 != -1 && id2 != -1) {
        addConnection(id1, id2);
        char status[128];
        sprintf(status, "Status: You are now friends with '%s'.", user2_name);
        gtk_label_set_text(GTK_LABEL(status_label), status);
        gtk_widget_queue_draw(drawing_area);
        gtk_widget_destroy(g_object_get_data(G_OBJECT(button), "list_box_row"));
    }
}

static void on_rec_view_profile_clicked(GtkButton *button, gpointer user_data) {
    const char *name = (const char*)g_object_get_data(G_OBJECT(button), "user_name");
    show_profile_dialog(name);
}

static void on_recommend_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *name_entry = g_object_get_data(G_OBJECT(button), "name_entry");
    const char* name = gtk_entry_get_text(GTK_ENTRY(name_entry));
    int id = findUserIdByName(name);
    GtkWidget* recommendation_listbox = g_object_get_data(G_OBJECT(button), "listbox");

    GList *children, *iter;
    children = gtk_container_get_children(GTK_CONTAINER(recommendation_listbox));
    for(iter = children; iter != NULL; iter = g_list_next(iter)) gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    if (id == -1) { gtk_label_set_text(GTK_LABEL(status_label), "Status: User not found for recommendations."); return; }
    int rec_count = 0;
    RecNode* recs = get_recommendations(id, &rec_count);
    if (rec_count == 0) {
        GtkWidget *label = gtk_label_new("No new recommendations found.");
        gtk_list_box_insert(GTK_LIST_BOX(recommendation_listbox), label, -1);
    } else {
        for (int i = 0; i < rec_count; i++) {
            int rec_id = recs[i].userId;
            GtkWidget *card_main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
            GtkWidget *icon = gtk_image_new_from_icon_name("avatar-default", GTK_ICON_SIZE_DIALOG);
            GtkWidget *card_text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
            char name_markup[100];
            sprintf(name_markup, "<b>%s</b>", profiles[rec_id].name);
            GtkWidget *name_label = gtk_label_new(NULL);
            gtk_label_set_markup(GTK_LABEL(name_label), name_markup);
            gtk_widget_set_halign(name_label, GTK_ALIGN_START);
            char reason_text[200];
            sprintf(reason_text, "<small>You both know: %s</small>", recs[i].mutualFriendNames);
            GtkWidget *reason_label = gtk_label_new(NULL);
            gtk_label_set_markup(GTK_LABEL(reason_label), reason_text);
            gtk_widget_set_halign(reason_label, GTK_ALIGN_START);
            GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
            GtkWidget *add_button = gtk_button_new_with_label("Add Friend");
            GtkWidget *profile_button = gtk_button_new_with_label("View Profile");
            GtkWidget *skip_button = gtk_button_new_with_label("Skip");
            GtkWidget *list_box_row = gtk_list_box_row_new();
            g_object_set_data(G_OBJECT(add_button), "list_box_row", list_box_row);
            g_object_set_data(G_OBJECT(add_button), "user1_name", (gpointer)name);
            g_object_set_data(G_OBJECT(add_button), "user2_name", (gpointer)profiles[rec_id].name);
            g_signal_connect(add_button, "clicked", G_CALLBACK(on_rec_add_friend_clicked), NULL);
            g_object_set_data(G_OBJECT(profile_button), "user_name", (gpointer)profiles[rec_id].name);
            g_signal_connect(profile_button, "clicked", G_CALLBACK(on_rec_view_profile_clicked), NULL);
            g_signal_connect(skip_button, "clicked", G_CALLBACK(on_rec_skip_clicked), list_box_row);
            gtk_box_pack_start(GTK_BOX(button_box), add_button, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(button_box), profile_button, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(button_box), skip_button, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(card_text_box), name_label, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(card_text_box), reason_label, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(card_text_box), button_box, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(card_main_box), icon, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(card_main_box), card_text_box, TRUE, TRUE, 0);
            gtk_container_add(GTK_CONTAINER(list_box_row), card_main_box);
            gtk_container_add(GTK_CONTAINER(recommendation_listbox), list_box_row);
        }
    }
    free(recs);
    gtk_widget_show_all(recommendation_listbox);
    char status[128];
    sprintf(status, "Status: Showing top recommendations for '%s'.", name);
    gtk_label_set_text(GTK_LABEL(status_label), status);
}

static void on_main_view_profile_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *name_entry = GTK_WIDGET(user_data);
    const char *name = gtk_entry_get_text(GTK_ENTRY(name_entry));
    if (strlen(name) > 0) {
        show_profile_dialog(name);
    } else {
        gtk_label_set_text(GTK_LABEL(status_label), "Status: Please enter a name to view profile.");
    }
}

static void on_delete_connection_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *name1_entry = g_object_get_data(G_OBJECT(button), "name1_entry");
    GtkWidget *name2_entry = g_object_get_data(G_OBJECT(button), "name2_entry");
    const char *name1 = gtk_entry_get_text(GTK_ENTRY(name1_entry));
    const char *name2 = gtk_entry_get_text(GTK_ENTRY(name2_entry));
    int id1 = findUserIdByName(name1);
    int id2 = findUserIdByName(name2);
    if (id1 == -1 || id2 == -1) { gtk_label_set_text(GTK_LABEL(status_label), "Status: One or both users not found."); return; }
    deleteConnection(id1, id2);
    char status[128];
    sprintf(status, "Status: Connection between '%s' and '%s' deleted.", name1, name2);
    gtk_label_set_text(GTK_LABEL(status_label), status);
    gtk_widget_queue_draw(drawing_area);
}

static void on_delete_user_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *name_entry = g_object_get_data(G_OBJECT(button), "name_entry");
    const char *name = gtk_entry_get_text(GTK_ENTRY(name_entry));
    int id = findUserIdByName(name);
    if (id == -1) { gtk_label_set_text(GTK_LABEL(status_label), "Status: User to delete not found."); return; }
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_YES_NO,
                                               "Are you sure you want to delete user '%s'?", name);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "This action cannot be undone.");
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (response == GTK_RESPONSE_YES) {
        deletenode(id);
        char status[128];
        sprintf(status, "Status: User '%s' has been deleted.", name);
        gtk_label_set_text(GTK_LABEL(status_label), status);
        gtk_widget_queue_draw(drawing_area);
    } else {
        gtk_label_set_text(GTK_LABEL(status_label), "Status: Deletion cancelled.");
    }
}

static void on_manage_remove_friend_clicked(GtkButton *button, gpointer user_data) {
    const char* user_name = g_object_get_data(G_OBJECT(button), "user_name");
    const char* friend_name = g_object_get_data(G_OBJECT(button), "friend_name");
    int user_id = findUserIdByName(user_name);
    int friend_id = findUserIdByName(friend_name);

    if (user_id != -1 && friend_id != -1) {
        deleteConnection(user_id, friend_id);
        char status[128];
        sprintf(status, "Status: Unfriended '%s'.", friend_name);
        gtk_label_set_text(GTK_LABEL(status_label), status);
        gtk_widget_queue_draw(drawing_area);
        gtk_widget_destroy(GTK_WIDGET(g_object_get_data(G_OBJECT(button), "row")));
    }
}

static void on_manage_view_profile_clicked(GtkButton *button, gpointer user_data) {
    const char* friend_name = g_object_get_data(G_OBJECT(button), "friend_name");
    show_profile_dialog(friend_name);
}

static void on_manage_friends_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *entry = GTK_WIDGET(user_data);
    const char *name = gtk_entry_get_text(GTK_ENTRY(entry));
    int id = findUserIdByName(name);
    if (id == -1) {
        gtk_label_set_text(GTK_LABEL(status_label), "Status: User not found.");
        return;
    }

    char title[100];
    sprintf(title, "Managing Friends for %s", name);
    GtkWidget *dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(main_window),
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "_Close", GTK_RESPONSE_CLOSE, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *list_box = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), list_box);
    gtk_box_pack_start(GTK_BOX(content_area), scrolled_window, TRUE, TRUE, 0);

    Node *temp = adjlist[id];
    if (temp == NULL) {
        GtkWidget *label = gtk_label_new("This user has no friends.");
        gtk_container_add(GTK_CONTAINER(list_box), label);
    }
    while (temp != NULL) {
        int friend_id = temp->vertex;
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        GtkWidget *icon = gtk_image_new_from_icon_name("avatar-default", GTK_ICON_SIZE_BUTTON);
        GtkWidget *name_label = gtk_label_new(profiles[friend_id].name);
        GtkWidget *button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        GtkWidget *remove_button = gtk_button_new_with_label("Remove");
        GtkWidget *view_button = gtk_button_new_with_label("View Profile");
        
        g_object_set_data(G_OBJECT(remove_button), "user_name", (gpointer)name);
        g_object_set_data(G_OBJECT(remove_button), "friend_name", (gpointer)profiles[friend_id].name);
        g_object_set_data(G_OBJECT(remove_button), "row", row);
        g_signal_connect(remove_button, "clicked", G_CALLBACK(on_manage_remove_friend_clicked), NULL);

        g_object_set_data(G_OBJECT(view_button), "friend_name", (gpointer)profiles[friend_id].name);
        g_signal_connect(view_button, "clicked", G_CALLBACK(on_manage_view_profile_clicked), NULL);

        gtk_box_pack_start(GTK_BOX(button_box), view_button, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(button_box), remove_button, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(row), icon, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row), name_label, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(row), button_box, TRUE, TRUE, 0);
        
        gtk_container_add(GTK_CONTAINER(list_box), row);
        temp = temp->next;
    }

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/**
 * @brief This function builds the entire user interface.
 */
static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window, *grid, *frame_reg, *grid_reg, *frame_actions, *grid_actions, *frame_view, *frame_rec, *grid_rec, *frame_manage;
    
    window = gtk_application_window_new(app);
    main_window = window;
    gtk_window_set_title(GTK_WINDOW(window), "Social Network Graph");
    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 800);
    g_signal_connect(window, "destroy", G_CALLBACK(cleanupNetwork), NULL);

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 15);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 15);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_container_add(GTK_CONTAINER(window), grid);

    GtkWidget *controls_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_vexpand(controls_box, TRUE);
    gtk_grid_attach(GTK_GRID(grid), controls_box, 0, 0, 1, 1);

    // -- Registration Frame --
    frame_reg = gtk_frame_new("User Management");
    gtk_box_pack_start(GTK_BOX(controls_box), frame_reg, FALSE, FALSE, 0);
    grid_reg = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid_reg), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid_reg), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid_reg), 10);
    gtk_container_add(GTK_CONTAINER(frame_reg), grid_reg);
    GtkWidget *label_name = gtk_label_new("Name:"); GtkWidget *entry_name = gtk_entry_new();
    GtkWidget *label_age = gtk_label_new("Age:"); GtkWidget *entry_age = gtk_entry_new();
    GtkWidget *label_email = gtk_label_new("Email:"); GtkWidget *entry_email = gtk_entry_new();
    GtkWidget *label_city = gtk_label_new("City:"); GtkWidget *entry_city = gtk_entry_new();
    GtkWidget *button_reg = gtk_button_new_with_label("Register User");
    gtk_grid_attach(GTK_GRID(grid_reg), label_name, 0, 0, 1, 1); gtk_grid_attach(GTK_GRID(grid_reg), entry_name, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_reg), label_age, 0, 1, 1, 1); gtk_grid_attach(GTK_GRID(grid_reg), entry_age, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_reg), label_email, 0, 2, 1, 1); gtk_grid_attach(GTK_GRID(grid_reg), entry_email, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_reg), label_city, 0, 3, 1, 1); gtk_grid_attach(GTK_GRID(grid_reg), entry_city, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_reg), button_reg, 0, 4, 2, 1);
    g_object_set_data(G_OBJECT(button_reg), "name_entry", entry_name);
    g_object_set_data(G_OBJECT(button_reg), "age_entry", entry_age);
    g_object_set_data(G_OBJECT(button_reg), "email_entry", entry_email);
    g_object_set_data(G_OBJECT(button_reg), "city_entry", entry_city);
    g_signal_connect(button_reg, "clicked", G_CALLBACK(on_register_clicked), NULL);

    // -- Connection & Delete Frame --
    frame_actions = gtk_frame_new("Connections & Actions");
    gtk_box_pack_start(GTK_BOX(controls_box), frame_actions, FALSE, FALSE, 0);
    grid_actions = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid_actions), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid_actions), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid_actions), 10);
    gtk_container_add(GTK_CONTAINER(frame_actions), grid_actions);
    GtkWidget *label_conn1 = gtk_label_new("User 1:"); GtkWidget *entry_conn1 = gtk_entry_new();
    GtkWidget *label_conn2 = gtk_label_new("User 2:"); GtkWidget *entry_conn2 = gtk_entry_new();
    GtkWidget *button_conn = gtk_button_new_with_label("Add");
    GtkWidget *button_del_conn = gtk_button_new_with_label("Delete");
    GtkWidget *conn_buttons_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(conn_buttons_box), button_conn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(conn_buttons_box), button_del_conn, TRUE, TRUE, 0);
    GtkWidget *label_del_user = gtk_label_new("User Name:"); GtkWidget *entry_del_user = gtk_entry_new();
    GtkWidget *button_del_user = gtk_button_new_with_label("Delete User");
    gtk_grid_attach(GTK_GRID(grid_actions), label_conn1, 0, 0, 1, 1); gtk_grid_attach(GTK_GRID(grid_actions), entry_conn1, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_actions), label_conn2, 0, 1, 1, 1); gtk_grid_attach(GTK_GRID(grid_actions), entry_conn2, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_actions), conn_buttons_box, 0, 2, 2, 1);
    gtk_grid_attach(GTK_GRID(grid_actions), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, 3, 2, 1);
    gtk_grid_attach(GTK_GRID(grid_actions), label_del_user, 0, 4, 1, 1); gtk_grid_attach(GTK_GRID(grid_actions), entry_del_user, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_actions), button_del_user, 0, 5, 2, 1);
    g_object_set_data(G_OBJECT(button_conn), "name1_entry", entry_conn1);
    g_object_set_data(G_OBJECT(button_conn), "name2_entry", entry_conn2);
    g_signal_connect(button_conn, "clicked", G_CALLBACK(on_connect_clicked), NULL);
    g_object_set_data(G_OBJECT(button_del_conn), "name1_entry", entry_conn1);
    g_object_set_data(G_OBJECT(button_del_conn), "name2_entry", entry_conn2);
    g_signal_connect(button_del_conn, "clicked", G_CALLBACK(on_delete_connection_clicked), NULL);
    g_object_set_data(G_OBJECT(button_del_user), "name_entry", entry_del_user);
    g_signal_connect(button_del_user, "clicked", G_CALLBACK(on_delete_user_clicked), NULL);
    
    // -- View Profile & Manage Friends Frame --
    frame_manage = gtk_frame_new("User Features");
    gtk_box_pack_start(GTK_BOX(controls_box), frame_manage, FALSE, FALSE, 0);
    GtkWidget *grid_manage = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(grid_manage), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid_manage), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid_manage), 10);
    gtk_container_add(GTK_CONTAINER(frame_manage), grid_manage);
    GtkWidget *label_manage = gtk_label_new("User Name:");
    GtkWidget *entry_manage = gtk_entry_new();
    GtkWidget *view_profile_button = gtk_button_new_with_label("View Profile");
    GtkWidget *manage_friends_button = gtk_button_new_with_label("Manage Friends");
    gtk_grid_attach(GTK_GRID(grid_manage), label_manage, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_manage), entry_manage, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_manage), view_profile_button, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_manage), manage_friends_button, 1, 1, 1, 1);
    g_signal_connect(view_profile_button, "clicked", G_CALLBACK(on_main_view_profile_clicked), entry_manage);
    g_signal_connect(manage_friends_button, "clicked", G_CALLBACK(on_manage_friends_clicked), entry_manage);

    // -- Recommendation Frame --
    frame_rec = gtk_frame_new("Friend Recommendations");
    gtk_box_pack_start(GTK_BOX(controls_box), frame_rec, TRUE, TRUE, 0);
    grid_rec = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid_rec), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid_rec), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid_rec), 10);
    gtk_container_add(GTK_CONTAINER(frame_rec), grid_rec);
    GtkWidget *label_rec = gtk_label_new("Your Name:");
    GtkWidget *entry_rec = gtk_entry_new();
    GtkWidget *button_rec = gtk_button_new_with_label("Find Recommendations");
    GtkWidget *recommendation_listbox = gtk_list_box_new();
    GtkWidget *rec_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(rec_scrolled_window, TRUE);
    gtk_container_add(GTK_CONTAINER(rec_scrolled_window), recommendation_listbox);
    gtk_grid_attach(GTK_GRID(grid_rec), label_rec, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_rec), entry_rec, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_rec), button_rec, 0, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid_rec), rec_scrolled_window, 0, 2, 2, 1);
    g_object_set_data(G_OBJECT(button_rec), "name_entry", entry_rec);
    g_object_set_data(G_OBJECT(button_rec), "listbox", recommendation_listbox);
    g_signal_connect(button_rec, "clicked", G_CALLBACK(on_recommend_clicked), NULL);

    // ---- Graph Visualization (Right Side) ----
    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(drawing_area, TRUE);
    gtk_widget_set_vexpand(drawing_area, TRUE);
    gtk_grid_attach(GTK_GRID(grid), drawing_area, 1, 0, 1, 1);
    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(draw_graph), NULL);
    
    // ---- Status Bar (Bottom) ----
    status_label = gtk_label_new("Status: Welcome! Please register a user to begin.");
    gtk_widget_set_halign(status_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), status_label, 0, 1, 2, 1);
    
    gtk_widget_show_all(window);
}

// ---- MAIN FUNCTION ----
int main(int argc, char **argv) {
    initNetwork(10);
    GtkApplication *app;
    int status;
    app = gtk_application_new("com.socialnetwork.app", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}