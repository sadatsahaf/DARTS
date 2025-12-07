#include "../../include/ds.h"

// --- Linked List Implementation ---
void list_add(Node** head, int fd) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->socket_fd = fd;
    new_node->next = *head;
    *head = new_node;
}

void list_remove(Node** head, int fd) {
    Node *temp = *head, *prev = NULL;
    if (temp != NULL && temp->socket_fd == fd) {
        *head = temp->next;
        free(temp);
        return;
    }
    while (temp != NULL && temp->socket_fd != fd) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) return;
    prev->next = temp->next;
    free(temp);
}

// --- Queue Implementation ---
void queue_init(Queue* q) {
    q->front = q->rear = NULL;
    q->count = 0;
}

void queue_push(Queue* q, Packet p) {
    QueueNode* temp = (QueueNode*)malloc(sizeof(QueueNode));
    temp->packet = p;
    temp->next = NULL;
    if (q->rear == NULL) {
        q->front = q->rear = temp;
    } else {
        q->rear->next = temp;
        q->rear = temp;
    }
    q->count++;
}

Packet queue_pop(Queue* q) {
    if (q->front == NULL) {
        Packet empty; memset(&empty, 0, sizeof(Packet)); return empty;
    }
    QueueNode* temp = q->front;
    Packet p = temp->packet;
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;
    free(temp);
    q->count--;
    return p;
}

int queue_is_empty(Queue* q) {
    return (q->front == NULL);
}

// --- Hash Table Implementation ---
void hash_init(HashTable* ht) {
    for(int i=0; i<HASH_SIZE; i++) ht->buckets[i] = NULL;
}

unsigned int hash_func(const char* key) {
    unsigned int hash = 5381;
    int c;
    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash % HASH_SIZE;
}

void hash_insert(HashTable* ht, const char* username, int fd) {
    unsigned int index = hash_func(username);
    HashEntry* new_entry = (HashEntry*)malloc(sizeof(HashEntry));
    strcpy(new_entry->username, username);
    new_entry->socket_fd = fd;
    new_entry->next = ht->buckets[index];
    ht->buckets[index] = new_entry;
}

int hash_get(HashTable* ht, const char* username) {
    unsigned int index = hash_func(username);
    HashEntry* entry = ht->buckets[index];
    while(entry != NULL) {
        if(strcmp(entry->username, username) == 0)
            return entry->socket_fd;
        entry = entry->next;
    }
    return -1;
}

void hash_remove(HashTable* ht, const char* username) {
    unsigned int index = hash_func(username);
    HashEntry *entry = ht->buckets[index], *prev = NULL;
    while(entry != NULL) {
        if(strcmp(entry->username, username) == 0) {
            if(prev == NULL) ht->buckets[index] = entry->next;
            else prev->next = entry->next;
            free(entry);
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

// Inefficient but necessary for list users / cleanup if we only have FD
char* hash_get_user_by_fd(HashTable* ht, int fd) {
    for(int i=0; i<HASH_SIZE; i++) {
        HashEntry* entry = ht->buckets[i];
        while(entry) {
            if(entry->socket_fd == fd) return entry->username;
            entry = entry->next;
        }
    }
    return NULL;
}