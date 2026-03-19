#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "monitor/monitor.h"
#include <string.h>
#include <stdio.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
P* new_wp() {
  if (free_ == NULL) {
    assert(0);
  }
  WP *wp = free_;
  free_ = free_->next;
  wp->next = NULL;
  return wp;
}

void free_wp(WP *wp) {
  wp->next = free_;
  free_ = wp;
}

WP* add_watchpoint(const char *e) {
  if (e == NULL) {
    return NULL;
  }
  while (*e == ' ') { e++; }
  if (*e == '\0') {
    return NULL;
  }

  WP *wp = new_wp();
  assert(strlen(e) < sizeof(wp->expr));
  strcpy(wp->expr, e);

  bool success = true;
  uint32_t val = expr(wp->expr, &success);
  if (!success) {
    printf("Bad expression: %s\n", wp->expr);
    free_wp(wp);
    return NULL;
  }
  wp->last_val = val;

  wp->next = head;
  head = wp;
  return wp;
}

bool delete_watchpoint(int no) {
  WP *prev = NULL;
  WP *cur = head;
  while (cur != NULL) {
    if (cur->NO == no) {
      if (prev == NULL) {
        head = cur->next;
      } else {
        prev->next = cur->next;
      }
      free_wp(cur);
      return true;
    }
    prev = cur;
    cur = cur->next;
  }
  return false;
}

void list_watchpoints() {
  if (head == NULL) {
    printf("No watchpoints.\n");
    return;
  }
  printf("Num  What\n");
  for (WP *wp = head; wp != NULL; wp = wp->next) {
    printf("%-4d %s\n", wp->NO, wp->expr);
  }
}

bool check_watchpoints() {
  bool triggered = false;
  for (WP *wp = head; wp != NULL; wp = wp->next) {
    bool success = true;
    uint32_t val = expr(wp->expr, &success);
    if (!success) {
      printf("Bad expression in watchpoint %d: %s\n", wp->NO, wp->expr);
      nemu_state = NEMU_STOP;
      return true;
    }
    if (val != wp->last_val) {
      printf("Watchpoint %d: %s\n", wp->NO, wp->expr);
      printf("Old value = 0x%08x\n", wp->last_val);
      printf("New value = 0x%08x\n", val);
      wp->last_val = val;
      triggered = true;
    }
  }
  if (triggered) {
    nemu_state = NEMU_STOP;
  }
  return triggered;
}

