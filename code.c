
/*
 emotion_final.c
 Fully-featured Emotion Path Optimizer (C)
  - Pre-filled tips & procedures for emotions
  - Some direct transitions forbidden (e.g., overwhelmed -> happy)
  - Friendly UI (no raw numbers shown)
  - Persistent save/load to "emotion_data.txt" (simple text format)
  - Multi-question assessment, add tips/procedures, run optimizer

 Save format (emotion_data.txt):
   NODE <name> <valence> <baseline>
   TIP  <emotion_name> "<tip text>"
   EDGE <from> <to> <weight> "<procedure>"

 Compile:
   gcc -std=c99 -Wall -O2 emotion_final_cleaned.c -o emo_tool

 Run:
   ./emo_tool
*/

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <ctype.h>

#define MAX_NAME_LEN 48
#define INIT_CAP 4
#define MAX_LINE 1024
#define SAVE_FILE "emotion_data.txt"

typedef struct { char *text; } Tip;

typedef struct Edge {
    int to;
    float weight;
    char *procedure;
} Edge;

typedef struct {
    char name[MAX_NAME_LEN];
    float valence;                 // internal only
    float baseline_intensity;      // internal only
    Edge *edges;
    int edges_count;
    int edges_cap;
    Tip *tips;
    int tips_count;
    int tips_cap;
} EmotionNode;

typedef struct {
    EmotionNode *nodes;
    int count;
    int cap;
} EmotionGraph;

/* ---------- Utility helpers ---------- */

static char *strdup_s(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (!d) { perror("malloc"); exit(1); }
    memcpy(d, s, n);
    return d;
}
static void *realloc_or_die(void *p, size_t nmemb, size_t size) {
    void *q = realloc(p, nmemb * size);
    if (!q && nmemb > 0) { perror("realloc"); exit(1); }
    return q;
}
static void ensure_graph_capacity(EmotionGraph *g) {
    if (g->count >= g->cap) {
        int newcap = (g->cap == 0) ? 8 : g->cap * 2;
        g->nodes = realloc_or_die(g->nodes, newcap, sizeof(EmotionNode));
        g->cap = newcap;
    }
}
static void ensure_edge_capacity(EmotionNode *n) {
    if (n->edges_count >= n->edges_cap) {
        int newcap = (n->edges_cap == 0) ? INIT_CAP : n->edges_cap * 2;
        n->edges = realloc_or_die(n->edges, newcap, sizeof(Edge));
        n->edges_cap = newcap;
    }
}
static void ensure_tip_capacity(EmotionNode *n) {
    if (n->tips_count >= n->tips_cap) {
        int newcap = (n->tips_cap == 0) ? INIT_CAP : n->tips_cap * 2;
        n->tips = realloc_or_die(n->tips, newcap, sizeof(Tip));
        n->tips_cap = newcap;
    }
}

/* ---------- Graph ops ---------- */

EmotionGraph *graph_new() {
    EmotionGraph *g = malloc(sizeof(EmotionGraph));
    if (!g) { perror("malloc"); exit(1); }
    g->nodes = NULL; g->count = 0; g->cap = 0;
    return g;
}

int graph_find(EmotionGraph *g, const char *name) {
    for (int i = 0; i < g->count; ++i) if (strcmp(g->nodes[i].name, name) == 0) return i;
    return -1;
}

int graph_add_node(EmotionGraph *g, const char *name, float valence, float baseline) {
    int idx = graph_find(g, name);
    if (idx != -1) return idx;
    ensure_graph_capacity(g);
    EmotionNode *n = &g->nodes[g->count];
    strncpy(n->name, name, MAX_NAME_LEN-1);
    n->name[MAX_NAME_LEN-1] = '\0';
    n->valence = valence;
    n->baseline_intensity = baseline;
    n->edges = NULL; n->edges_count = 0; n->edges_cap = 0;
    n->tips = NULL; n->tips_count = 0; n->tips_cap = 0;
    return g->count++;
}

/* Forbidden direct transitions: emotions that should NOT connect directly to positive goals.
   Example: "overwhelmed" should not connect directly to "happy"/"calm"/"hopeful"/"peaceful".
   graph_add_edge respects this check when 'filter_direct_positive' is 1.
*/
int is_positive_goal_name(const char *name) {
    const char *goals[] = {"happy","calm","hopeful","peaceful"};
    for (int i=0;i<4;++i) if (strcmp(name, goals[i])==0) return 1;
    return 0;
}

void graph_add_edge(EmotionGraph *g, const char *from, const char *to, float weight, const char *procedure) {
    // Enforce forbidden direct connections for "overwhelmed"
    if (strcmp(from, "overwhelmed") == 0 && is_positive_goal_name(to)) {
        // refuse direct edge - do not add
        // The system prefers routing overwhelmed -> grounded -> calm/happy
        return;
    }

    int u = graph_find(g, from);
    if (u == -1) u = graph_add_node(g, from, -0.5f, 5.0f);
    int v = graph_find(g, to);
    if (v == -1) v = graph_add_node(g, to, 0.0f, 5.0f);

    EmotionNode *nu = &g->nodes[u];
    ensure_edge_capacity(nu);
    Edge *e = &nu->edges[nu->edges_count++];
    e->to = v;
    e->weight = (weight < 0.0f) ? 0.0f : weight;
    e->procedure = procedure ? strdup_s(procedure) : NULL;

    // Add reverse edge for undirected feel (reverse procedure not set)
    EmotionNode *nv = &g->nodes[v];
    ensure_edge_capacity(nv);
    Edge *er = &nv->edges[nv->edges_count++];
    er->to = u;
    er->weight = (weight < 0.0f) ? 0.0f : weight;
    er->procedure = NULL;
}

void graph_add_tip(EmotionGraph *g, const char *emotion, const char *tip_text) {
    int idx = graph_find(g, emotion);
    if (idx == -1) idx = graph_add_node(g, emotion, -0.2f, 5.0f);
    EmotionNode *n = &g->nodes[idx];
    ensure_tip_capacity(n);
    n->tips[n->tips_count++].text = strdup_s(tip_text);
}

/* ---------- Friendly printing (no internals) ---------- */

void graph_print_friendly(EmotionGraph *g) {
    printf("Current emotional map (%d emotions):\n", g->count);
    for (int i=0;i<g->count;++i) {
        EmotionNode *n = &g->nodes[i];
        printf(" - %s", n->name);
        if (n->tips_count > 0) printf("  (tips: %d)", n->tips_count);
        printf("\n");
        for (int e=0;e<n->edges_count;++e) {
            printf("     -> %s", g->nodes[n->edges[e].to].name);
            if (n->edges[e].procedure) printf("  (action)");
            printf("\n");
        }
    }
}

/* ---------- ASCII graph visualization ---------- */

void graph_print_ascii(EmotionGraph *g) {
    printf("\n=== ASCII Graph View ===\n");
    for (int i = 0; i < g->count; i++) {
        EmotionNode *n = &g->nodes[i];
        printf("\n[%s]\n", n->name);

        if (n->tips_count > 0) {
            printf("  Tips:\n");
            for (int t = 0; t < n->tips_count; ++t) {
                printf("    - %s\n", n->tips[t].text);
            }
        }

        if (n->edges_count == 0) {
            printf("  (no connections)\n");
            continue;
        }

        for (int e = 0; e < n->edges_count; e++) {
            Edge *ed = &n->edges[e];
            printf("   |-- %s", g->nodes[ed->to].name);
            if (ed->procedure)
                printf("  (action: %s)", ed->procedure);
            printf("  [weight: %.2f]\n", ed->weight);
        }
    }
    printf("\n=========================\n");
}

/* ---------- Assessment prototypes & selection ---------- */

typedef struct { char name[MAX_NAME_LEN]; float stress, overwhelm, anger, sadness; } Prototype;

int choose_closest_prototype(float s, float o, float a, float sd, Prototype *protos, int pcount) {
    float bestd = FLT_MAX; int best = 0;
    for (int i=0;i<pcount;++i) {
        float ds = s - protos[i].stress;
        float doo = o - protos[i].overwhelm;
        float da = a - protos[i].anger;
        float dsd = sd - protos[i].sadness;
        float dist = ds*ds + doo*doo + da*da + dsd*dsd;
        if (dist < bestd) { bestd = dist; best = i; }
    }
    return best;
}

/* ---------- Personalized Dijkstra (O(n^2)) ----------
   Internal details are kept inside; user sees only the final path and actions.
   Small heuristics:
    - Nodes that have user tips are slightly favored (treated as easier)
    - Edges with procedures are slightly favored
    - Some nodes are blocked from direct positive edges (handled when adding edges)
*/

float run_dijkstra_personalized(EmotionGraph *g, int src, int dest, int out_path[], int *out_len) {
    int n = g->count;
    if (src < 0 || dest < 0) return FLT_MAX;
    float dist[n]; int visited[n]; int prev[n];
    for (int i=0;i<n;++i) { dist[i]=FLT_MAX; visited[i]=0; prev[i]=-1; }
    dist[src] = 0.0f;

    for (int iter=0; iter<n; ++iter) {
        int u=-1; float best=FLT_MAX;
        for (int i=0;i<n;++i) if (!visited[i] && dist[i] < best) { best = dist[i]; u = i; }
        if (u==-1) break;
        visited[u] = 1;
        if (u == dest) break;
        EmotionNode *nu = &g->nodes[u];
        for (int ei=0; ei < nu->edges_count; ++ei) {
            Edge *e = &nu->edges[ei];
            int v = e->to;
            if (visited[v]) continue;
            float w = e->weight;
            if (g->nodes[v].tips_count > 0) w *= 0.85f;
            if (e->procedure) w *= 0.8f;
            /* small internal bias from valence (hidden) */
            float valence_bias = (1.0f - g->nodes[v].valence) * 0.05f;
            w = w * (1.0f - valence_bias);
            float alt = dist[u] + w;
            if (alt < dist[v]) { dist[v] = alt; prev[v] = u; }
        }
    }

    if (dist[dest] == FLT_MAX) return FLT_MAX;
    int temp[n]; int idx=0;
    for (int cur = dest; cur != -1; cur = prev[cur]) temp[idx++] = cur;
    *out_len = idx;
    for (int i=0;i<idx;++i) out_path[i] = temp[idx-1-i];
    return dist[dest];
}

/* ---------- I/O helpers ---------- */

static void read_line_trim(char *buf, int size) {
    if (!fgets(buf, size, stdin)) { buf[0]='\0'; return; }
    size_t len = strlen(buf);
    if (len>0 && buf[len-1]=='\n') buf[len-1] = '\0';
    char *p=buf; while (*p && isspace((unsigned char)*p)) ++p;
    if (p != buf) memmove(buf, p, strlen(p)+1);
}
static int read_int_in_range(const char *prompt, int lo, int hi) {
    char buf[128]; int val;
    while (1) {
        printf("%s (%d-%d): ", prompt, lo, hi);
        read_line_trim(buf, sizeof(buf));
        if (sscanf(buf, "%d", &val) == 1 && val >= lo && val <= hi) return val;
        printf("  Please enter a number between %d and %d.\n", lo, hi);
    }
}

/* ---------- Persistence: save/load (format A) ---------- */

static void fwrite_quoted(FILE *f, const char *s) {
    fputc('"', f);
    for (; *s; ++s) {
        if (*s == '"' || *s == '\\') fputc('\\', f);
        fputc((unsigned char)*s, f);
    }
    fputc('"', f);
}

int save_graph(EmotionGraph *g, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) { perror("fopen"); return 0; }
    for (int i=0;i<g->count;++i) {
        EmotionNode *n = &g->nodes[i];
        fprintf(f, "NODE %s %.3f %.3f\n", n->name, n->valence, n->baseline_intensity);
        for (int t=0;t<n->tips_count;++t) {
            fprintf(f, "TIP %s ", n->name);
            fwrite_quoted(f, n->tips[t].text); fputc('\n', f);
        }
    }
    for (int i=0;i<g->count;++i) {
        EmotionNode *n = &g->nodes[i];
        for (int e=0;e<n->edges_count;++e) {
            int to = n->edges[e].to;
            if (i < to) { // avoid duplicate edges
                fprintf(f, "EDGE %s %s %.3f ", n->name, g->nodes[to].name, n->edges[e].weight);
                if (n->edges[e].procedure) fwrite_quoted(f, n->edges[e].procedure);
                else fprintf(f, "\"\"");
                fputc('\n', f);
            }
        }
    }
    fclose(f); return 1;
}

static char *parse_quoted(const char *s, const char **endptr) {
    if (!s || *s != '"') { if (endptr) *endptr = s; return NULL; }
    const char *p = s+1; char buf[MAX_LINE]; int bi=0;
    while (*p && *p != '"') {
        if (*p == '\\' && *(p+1)) { p++; buf[bi++] = *p++; }
        else buf[bi++] = *p++;
        if (bi >= MAX_LINE-1) break;
    }
    buf[bi]='\0'; if (*p == '"') p++;
    if (endptr) *endptr = p;
    return strdup_s(buf);
}

int load_graph(EmotionGraph *g, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        size_t ln = strlen(line); if (ln>0 && line[ln-1]=='\n') line[ln-1]='\0';
        char *p = line; while (*p && isspace((unsigned char)*p)) ++p;
        if (!*p || *p == '#') continue;
        char token[32];
        if (sscanf(p, "%31s", token) != 1) continue;
        if (strcmp(token, "NODE") == 0) {
            p += 4; while (*p && isspace((unsigned char)*p)) ++p;
            char name[MAX_NAME_LEN]; float val=0.0f, base=5.0f;
            if (sscanf(p, "%47s %f %f", name, &val, &base) >= 1) {
                graph_add_node(g, name, val, base);
                int idx = graph_find(g, name);
                if (idx != -1) {
                    /* attempt more precise parse */
                    char tmp[MAX_LINE]; strncpy(tmp, p, sizeof(tmp)); tmp[sizeof(tmp)-1]=0;
                    char *tok = strtok(tmp, " \t");
                    if (tok) { tok = strtok(NULL, " \t"); if (tok) g->nodes[idx].valence = atof(tok); tok = strtok(NULL, " \t"); if (tok) g->nodes[idx].baseline_intensity = atof(tok); }
                }
            }
        } else if (strcmp(token, "TIP") == 0) {
            p += 3; while (*p && isspace((unsigned char)*p)) ++p;
            char emo[MAX_NAME_LEN];
            if (sscanf(p, "%47s", emo) == 1) {
                char *q = strchr(p, '"');
                if (q) {
                    const char *endptr; char *tip = parse_quoted(q, &endptr);
                    if (tip) { graph_add_tip(g, emo, tip); free(tip); }
                }
            }
        } else if (strcmp(token, "EDGE") == 0) {
            p += 4; while (*p && isspace((unsigned char)*p)) ++p;
            char from[MAX_NAME_LEN], to[MAX_NAME_LEN]; float w = 1.0f;
            if (sscanf(p, "%47s %47s %f", from, to, &w) >= 2) {
                char *q = strchr(p, '"'); char *proc = NULL;
                if (q) { const char *endptr; proc = parse_quoted(q, &endptr); }
                graph_add_edge(g, from, to, w, proc);
                if (proc) free(proc);
            }
        }
    }
    fclose(f); return 1;
}

/* ---------- Friendly explanation for the user ---------- */

void show_simple_explanation() {
    printf("\nHow this helper chooses a plan (short version):\n");
    printf("  - Emotions are like locations on a map.\n");
    printf("  - Each connection has a difficulty - some routes are easier.\n");
    printf("  - Your personal tips and actions make certain routes easier.\n");
    printf("  - The system finds the smoothest step-by-step route from how you feel now\n");
    printf("    to a nearby positive state (like calm or happy) and suggests actions.\n");
    printf("  - Some jumps are blocked for safety and realism - e.g., if you're overwhelmed\n");
    printf("    you first move to a grounding step before aiming for calm or happy.\n\n");
}

/* ---------- Interactive UI ---------- */

static void read_line_trim_wrapper(char *buf, int size) { read_line_trim(buf, size); }
static int read_int_in_range_wrapper(const char *prompt, int lo, int hi) { return read_int_in_range(prompt, lo, hi); }

void seed_defaults_if_empty(EmotionGraph *g) {
    if (g->count > 0) return;

    /* Add nodes with internal valence/baseline (not shown to user) */
    graph_add_node(g, "overwhelmed", -0.95f, 8.5f);
    graph_add_node(g, "anxious", -0.7f, 7.0f);
    graph_add_node(g, "frustrated", -0.6f, 6.0f);
    graph_add_node(g, "angry", -0.7f, 6.5f);
    graph_add_node(g, "sad", -0.8f, 6.0f);
    graph_add_node(g, "lonely", -0.5f, 5.0f);
    graph_add_node(g, "grounded", -0.1f, 3.0f);
    graph_add_node(g, "calm", 0.6f, 3.0f);
    graph_add_node(g, "hopeful", 0.7f, 2.5f);
    graph_add_node(g, "happy", 1.0f, 1.5f);
    graph_add_node(g, "peaceful", 0.9f, 1.5f);

    /* Add default tips for each node (at least one; many have more) */
    graph_add_tip(g, "overwhelmed", "Put your phone away and take 3 big slow breaths.");
    graph_add_tip(g, "overwhelmed", "Try: press your feet firmly into the ground for 30 seconds.");
    graph_add_tip(g, "anxious", "5 slow breaths (inhale 4s, hold 2s, exhale 6s).");
    graph_add_tip(g, "anxious", "Name 5 things you can see right now.");
    graph_add_tip(g, "frustrated", "Step away for 2 minutes and stretch.");
    graph_add_tip(g, "frustrated", "Count backwards from 20 slowly.");
    graph_add_tip(g, "angry", "Take a 60-second walk or do physical movement.");
    graph_add_tip(g, "sad", "Write 3 small things that went okay today.");
    graph_add_tip(g, "sad", "Call or message someone you trust - say 'I need a small favor'.");
    graph_add_tip(g, "lonely", "Try a brief message to a friend or online community.");
    graph_add_tip(g, "grounded", "Place an object in your hand and describe it slowly.");
    graph_add_tip(g, "calm", "Listen to a favorite 3-minute song.");
    graph_add_tip(g, "hopeful", "List one small goal for the next 24 hours.");
    graph_add_tip(g, "happy", "Celebrate: do one small reward for yourself.");
    graph_add_tip(g, "peaceful", "Try a 2-minute body scan relaxation.");

    /* Add edges (note: graph_add_edge will block direct overwhelmed -> positive) */
    graph_add_edge(g, "overwhelmed", "grounded", 1.0f, "5 grounding breaths & plant feet");
    graph_add_edge(g, "grounded", "calm", 1.5f, "2-minute breathing");
    graph_add_edge(g, "calm", "happy", 1.5f, "play a mood-lifting song");
    graph_add_edge(g, "calm", "peaceful", 1.0f, "gentle stretching");
    graph_add_edge(g, "anxious", "grounded", 1.2f, "5 slow breaths");
    graph_add_edge(g, "anxious", "frustrated", 1.8f, NULL);
    graph_add_edge(g, "frustrated", "calm", 1.5f, "count to 10 and stretch");
    graph_add_edge(g, "frustrated", "angry", 2.0f, "step away and breathe");
    graph_add_edge(g, "angry", "grounded", 2.0f, "walk for 2 minutes");
    graph_add_edge(g, "sad", "hopeful", 2.0f, "write 3 small wins");
    graph_add_edge(g, "lonely", "hopeful", 2.5f, "reach out to one person");
    /* Note: we do NOT add direct overwhelmed->happy/calm/hopeful edges. */

    /* Also add some cross-connections to allow multiple paths */
    graph_add_edge(g, "anxious", "sad", 1.7f, NULL);
    graph_add_edge(g, "sad", "calm", 2.5f, "sit with feelings and breathe");
    graph_add_edge(g, "hopeful", "happy", 1.0f, NULL);
}

/* ---------- Menu and flow ---------- */

void print_welcome() {
    printf("Welcome to the Emotion Path Helper - a calm, friendly assistant.\n");
    printf("This tool helps suggest a simple, step-by-step plan from how you feel now\n");
    printf("toward a more positive state. Your tips and actions are saved between runs.\n");
    printf("Data file: %s\n\n", SAVE_FILE);
    show_simple_explanation();
}

void interactive_menu(EmotionGraph *g) {
    seed_defaults_if_empty(g);
    int running = 1;
    char buf[512];

    Prototype protos[] = {
        {"anxious", 7, 6, 2, 3},
        {"sad", 3, 3, 1, 8},
        {"angry", 4, 2, 8, 2},
        {"overwhelmed", 8, 9, 3, 6},
        {"lonely", 2, 3, 1, 6},
        {"calm", 1, 1, 0, 0},
        {"hopeful", 1, 1, 0, 1},
        {"happy", 0, 0, 0, 0}
    };
    int proto_count = sizeof(protos)/sizeof(protos[0]);

    while (running) {
        printf("\n--- Menu ---\n");
        printf("  1) Multi-question check-in (recommended)\n");
        printf("  2) Quick: enter your emotion by name\n");
        printf("  3) List emotions & tips\n");
        printf("  4) Add a personal tip to an emotion\n");
        printf("  5) Add/Edit action for a transition\n");
        printf("  6) Save now\n");
        printf("  7) Reload saved data (discard unsaved changes)\n");
        printf("  8) Show ASCII graph view\n");
        printf("  0) Exit (auto-saves)\n");
        int choice = read_int_in_range("Choose option", 0, 8);

        if (choice == 0) { running = 0; }
        else if (choice == 1 || choice == 2) {
            int src_idx = -1;
            if (choice == 1) {
                printf("\nCheck-in: please rate the following 0 (none) to 10 (very high).\n");
                int stress = read_int_in_range("Stress", 0, 10);
                int overwhelm = read_int_in_range("Overwhelm", 0, 10);
                int anger = read_int_in_range("Anger", 0, 10);
                int sadness = read_int_in_range("Sadness", 0, 10);
                int pidx = choose_closest_prototype((float)stress, (float)overwhelm, (float)anger, (float)sadness, protos, proto_count);
                const char *inferred = protos[pidx].name;
                printf("We think you may be feeling: %s\n", inferred);
                if (graph_find(g, inferred) == -1) graph_add_node(g, inferred, -0.2f, (stress+overwhelm)/2.0f);
                src_idx = graph_find(g, inferred);
            } else {
                printf("Enter your current emotion (e.g., anxious): ");
                char emo[MAX_NAME_LEN]; read_line_trim(emo, sizeof(emo));
                if (strlen(emo) == 0) { printf("No emotion entered.\n"); continue; }
                if (graph_find(g, emo) == -1) graph_add_node(g, emo, -0.2f, 5.0f);
                src_idx = graph_find(g, emo);
            }

            /* goals */
            const char *goals[] = {"happy","calm","peaceful","hopeful"};
            int gcount = 4, goal_idx[4];
            for (int i=0;i<gcount;++i) {
                goal_idx[i] = graph_find(g, goals[i]);
                if (goal_idx[i] == -1) { graph_add_node(g, goals[i], 0.9f, 2.0f); goal_idx[i] = graph_find(g, goals[i]); }
            }

            float best_cost = FLT_MAX; int best_goal=-1; int best_len=0; int best_path[128];
            int tmp_path[128], tmp_len;
            for (int i=0;i<gcount;++i) {
                float cost = run_dijkstra_personalized(g, src_idx, goal_idx[i], tmp_path, &tmp_len);
                if (cost < best_cost) { best_cost = cost; best_goal = goal_idx[i]; best_len = tmp_len; memcpy(best_path, tmp_path, sizeof(int)*tmp_len); }
            }

            if (best_cost == FLT_MAX) {
                printf("\nSorry - no available path to a positive state. Try adding tips or transitions in the menu.\n");
            } else {
                printf("\nHere is a simple step-by-step plan:\n");
                for (int i=0;i<best_len;i++) {
                    int idx = best_path[i];
                    printf(" Step %d: %s\n", i+1, g->nodes[idx].name);
                    if (g->nodes[idx].tips_count > 0) {
                        printf("   Tips:\n");
                        for (int t=0;t<g->nodes[idx].tips_count;++t) printf("     - %s\n", g->nodes[idx].tips[t].text);
                    }
                    if (i < best_len-1) {
                        /* find procedure */
                        EmotionNode *cur = &g->nodes[idx];
                        char *proc = NULL;
                        for (int e=0;e<cur->edges_count;++e) if (cur->edges[e].to == best_path[i+1]) { proc = cur->edges[e].procedure; break; }
                        if (proc) printf("   Action: %s\n", proc);
                        else printf("   Action: (none - you can add one in menu option 5)\n");
                    } else {
                        printf("   Goal reached: %s - well done for taking steps.\n", g->nodes[idx].name);
                    }
                }
            }
        } else if (choice == 3) {
            printf("\n");
            graph_print_friendly(g);
        } else if (choice == 4) {
            printf("\nAdd a personal tip.\nEmotion name: ");
            char emo[MAX_NAME_LEN]; read_line_trim(emo, sizeof(emo));
            if (strlen(emo)==0) { printf("No emotion entered.\n"); continue; }
            printf("Enter your tip (short): ");
            char tip_buf[512]; read_line_trim(tip_buf, sizeof(tip_buf));
            if (strlen(tip_buf)==0) { printf("No tip entered.\n"); continue; }
            graph_add_tip(g, emo, tip_buf);
            printf("Tip added to %s.\n", emo);
        } else if (choice == 5) {
            printf("\nAdd/Edit an action for a transition.\nFrom: "); char from[MAX_NAME_LEN]; read_line_trim(from, sizeof(from));
            printf("To: "); char to[MAX_NAME_LEN]; read_line_trim(to, sizeof(to));
            if (strlen(from)==0 || strlen(to)==0) { printf("Invalid names.\n"); continue; }
            /* blocked direct edges check: if from==\"overwhelmed\" and to positive, explain and offer to link via grounded */
            if (strcmp(from, "overwhelmed")==0 && is_positive_goal_name(to)) {
                printf("Direct transitions from 'overwhelmed' to positive states are blocked for safety.\n");
                printf("Would you like to create/inspect the path: overwhelmed -> grounded -> %s ? (y/n): ", to);
                char yn[8]; read_line_trim(yn, sizeof(yn));
                if (yn[0]=='y' || yn[0]=='Y') {
                    /* ensure edges exist */
                    graph_add_edge(g, "overwhelmed", "grounded", 1.0f, "5 grounding breaths & plant feet");
                    graph_add_edge(g, "grounded", to, 1.5f, NULL);
                    printf("Linked overwhelmed -> grounded -> %s. You can add actions on these transitions now.\n", to);
                } else { printf("No direct change made.\n"); }
                continue;
            }
            int u = graph_find(g, from); if (u==-1) u = graph_add_node(g, from, -0.2f, 5.0f);
            int v = graph_find(g, to); if (v==-1) v = graph_add_node(g, to, -0.2f, 5.0f);
            /* find existing edge u->v */
            EmotionNode *nu = &g->nodes[u];
            int found = -1;
            for (int e=0;e<nu->edges_count;++e) if (nu->edges[e].to == v) { found = e; break; }
            if (found == -1) {
                int w = read_int_in_range("Enter transition difficulty (0 = easy, bigger = harder)", 0, 20);
                printf("Enter action/procedure for this transition (blank for none):\n");
                char proc_buf[512]; read_line_trim(proc_buf, sizeof(proc_buf));
                graph_add_edge(g, from, to, (float)w, (strlen(proc_buf)>0)?proc_buf:NULL);
                printf("Edge created.\n");
            } else {
                printf("Existing transition found. Enter new action (blank to remove):\n");
                char proc_buf[512]; read_line_trim(proc_buf, sizeof(proc_buf));
                if (nu->edges[found].procedure) { free(nu->edges[found].procedure); nu->edges[found].procedure = NULL; }
                if (strlen(proc_buf) > 0) nu->edges[found].procedure = strdup_s(proc_buf);
                printf("Action updated.\n");
            }
        } else if (choice == 6) {
            if (save_graph(g, SAVE_FILE)) printf("Saved to %s.\n", SAVE_FILE);
            else printf("Save failed.\n");
        } else if (choice == 7) {
            printf("Reload from %s (discard unsaved changes)? (y/n): ", SAVE_FILE);
            char a[8]; read_line_trim(a, sizeof(a));
            if (a[0]=='y' || a[0]=='Y') {
                /* free current graph, reinit */
                for (int i=0;i<g->count;++i) {
                    EmotionNode *n = &g->nodes[i];
                    for (int e=0;e<n->edges_count;++e) if (n->edges[e].procedure) free(n->edges[e].procedure);
                    for (int t=0;t<n->tips_count;++t) if (n->tips[t].text) free(n->tips[t].text);
                    free(n->edges); free(n->tips);
                }
                free(g->nodes); g->nodes = NULL; g->count = 0; g->cap = 0;
                if (load_graph(g, SAVE_FILE)) printf("Reloaded from %s.\n", SAVE_FILE);
                else { printf("No save found; reset to defaults.\n"); seed_defaults_if_empty(g); }
            } else printf("Cancelled.\n");
        } else if (choice == 8) {
            graph_print_ascii(g);
        } else {
            printf("Unknown option.\n");
        }
    }
}

/* ---------- Cleanup ---------- */

void graph_free(EmotionGraph *g) {
    if (!g) return;
    for (int i=0;i<g->count;++i) {
        EmotionNode *n = &g->nodes[i];
        for (int e=0;e<n->edges_count;++e) if (n->edges[e].procedure) free(n->edges[e].procedure);
        for (int t=0;t<n->tips_count;++t) if (n->tips[t].text) free(n->tips[t].text);
        free(n->edges); free(n->tips);
    }
    free(g->nodes); free(g);
}

/* ---------- main ---------- */

int main(void) {
    EmotionGraph *g = graph_new();
    printf("Emotion Path Helper - friendly & private\n");
    if (load_graph(g, SAVE_FILE)) {
        printf("Loaded saved data from %s.\n", SAVE_FILE);
    } else {
        printf("No save found - starting with helpful defaults.\n");
    }
    print_welcome();
    interactive_menu(g);
    if (save_graph(g, SAVE_FILE)) printf("Auto-saved to %s.\n", SAVE_FILE);
    else printf("Auto-save failed.\n");
    graph_free(g);
    printf("Goodbye - take care!\n");
    return 0;
}
