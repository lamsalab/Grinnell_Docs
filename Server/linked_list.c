
typedef struct node {
  unsigned short username;
  struct node_t* next;
} node_t;

typedef struct list {
  node_t* head;
} list_t;

void list_init(list_t* list){
  list->head = NULL;
}

void list_add(list_t* list, unsigned short data) {
  node_t* cur = list->head;
  node_t* new = (node_t*)malloc(sizeof(node_t));
  new->username = data;
  new->next = list->head;
  list->head = new;
}

void list_remove(list_t* list, unsigned short data) {
  node_t* cur = list->head;
  node_t* prev = cur;
  
  while(cur!=NULL) {
    if(cur->username == data) {
      if (prev == cur) {
        cur = cur->next;
        free(prev);
        break;
      } else {
        prev->next = cur->next;
        free(cur);
        break;
      }
    }
    prev = cur;
    cur = cur->next;
  }
}

bool has_username (list_t* list, unsigned short data) {
  node_t* cur = list->head;

  while(cur!=NULL) {
    if(cur->username == data) {
      return true;
    }
    cur = cur->next;
  }
  return false;
}
