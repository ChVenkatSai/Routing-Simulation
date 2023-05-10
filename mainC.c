#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#pragma warning(disable : 4996)
#define MAX_NODES 10
#define INF INT_MAX
/* a rtpkt is the packet sent from one router to
   another*/
struct rtpkt {
    int sourceid;       /* id of sending router sending this pkt */
    int destid;         /* id of router to which pkt being sent
                           (must be an directly connected neighbor) */
    int* mincost;    /* min cost to all the node  */
};


struct distance_table
{
    int** costs;     // the distance table of curr_node, costs[i][j] is the cost from node i to node j
};

struct traffic {
    int** traf;
};

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 2 and below network environment:
  - emulates the transmission and delivery (with no loss and no
    corruption) between two physically connected nodes
  - calls the initializations routine rtinit once before
    beginning emulation for each node.

You should read and understand the code below. For Part A, you should fill all parts with annotation starting with "Todo". For Part B and Part C, you need to add additional routines for their features.
******************************************************************/
struct path {
    int node;
    struct path* next;
};



struct event {
    float evtime;           /* event time */
    int evtype;             /* event type code */
    int eventity;           /* entity (node) where event occurs */
    struct rtpkt* rtpktptr; /* ptr to packet (if any) assoc w/ this event */
    struct event* prev;
    struct event* next;
};
struct event* evlist = NULL;   /* the event list */
struct event* shadowlist = NULL;
struct path* path = NULL;
struct distance_table* dts;
struct traffic* traffic;
int simulation_slot = 0;
int** link_costs; /*This is a 2D matrix stroing the content defined in topo file*/
int num_nodes;
int num_tra;
int min_dist;
//int path[4];
int visited[MAX_NODES];
int fl[MAX_NODES];
int** link_traffic;
int** forwarding_table;
//visit.n1 = visit.n2 = visit.n3 = visit.n4 = FALSE;
#define MAXLINE 1024
/* possible events: */
#define INT_MAX 10000
/*Note in this lab, we only have one event, namely FROM_LAYER2.It refer to that the packet will pop out from layer3, you can add more event to emulate other activity for other layers. Like FROM_LAYER3*/
#define  FROM_LAYER2     1

float clocktime = 0.000;
void send2neighbor(struct rtpkt*);
void send2shadow(struct rtpkt*);
int flag = 0;
/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void rtinit(struct distance_table* dt, int node, int* link_costs, int num_nodes)
{
    int i = 0;
    /* Todo: Please write the code here*/
    dt->costs = (int**)malloc(num_nodes * sizeof(int*));
    if (dt->costs == NULL) {
        printf("Error: Memory allocation failed.\n");
        exit(1);
    }

    for (int j = 0; j < num_nodes; j++) {
        dt->costs[j] = (int*)malloc(num_nodes * sizeof(int));
        if (dt->costs[j] == NULL) {
            printf("Error: Memory allocation failed.\n");
            exit(1);
        }

    }

    for (int i = 0; i < num_nodes; i++) {
        for (int j = 0; j < num_nodes; j++) {
            dt->costs[i][j] = -1;
        }
    }

    for (int i = 0; i < num_nodes;i++) {
        dt->costs[node][i] = link_costs[i];
    }



    for (int j = 0;j < num_nodes; j++) {
        if (j != node && dt->costs[node][j] != -1) {
            struct rtpkt* pkt = (struct rtpkt*)malloc(sizeof(struct rtpkt));;
            pkt->sourceid = node;
            //intf("node %d", pkt->sourceid);
            pkt->mincost = (int*)malloc(num_nodes * sizeof(int));
            for (int i = 0; i < num_nodes; i++) {

                pkt->mincost[i] = dt->costs[node][i];

            }
            pkt->destid = j;
            send2neighbor(pkt);
        }
    }

}

void rtupdate(struct distance_table* dt, struct rtpkt recv_pkt)
{
    for (int j = 0; j < num_nodes; j++) {
        dt->costs[recv_pkt.sourceid][j] = recv_pkt.mincost[j];
    }
    //printf("\n");
    //printf("hi\n");
    int i = recv_pkt.destid;

    int count = 0;
    for (int j = 0;j < num_nodes;j++) {
        if (dt->costs[i][j] == -1 && recv_pkt.mincost[j] != -1 && dt->costs[i][recv_pkt.sourceid] != -1) {
            dt->costs[i][j] = dt->costs[i][recv_pkt.sourceid] + recv_pkt.mincost[j];
            forwarding_table[i][j] = recv_pkt.sourceid;
            count++;
        }
        else if ((dt->costs[i][j] > recv_pkt.mincost[j] + dt->costs[i][recv_pkt.sourceid]) && recv_pkt.mincost[j] != -1 && dt->costs[i][recv_pkt.sourceid] != -1) {
            dt->costs[i][j] = dt->costs[i][recv_pkt.sourceid] + recv_pkt.mincost[j];
            forwarding_table[i][j] = recv_pkt.sourceid;
            count++;
        }
    }

    if (i == recv_pkt.destid && count > 0) {
        for (int k = 0;k < num_nodes;k++) {
            if (dt->costs[i][k] != -1 && k != recv_pkt.destid) {
                struct rtpkt* pack_send = (struct rtpkt*)malloc(sizeof(struct rtpkt));
                pack_send->sourceid = recv_pkt.destid;
                pack_send->destid = k;
                pack_send->mincost = (int*)malloc(num_nodes * sizeof(int));
                for (int m = 0;m < num_nodes;m++) {
                    pack_send->mincost[m] = dt->costs[i][m];
                }
                //printf("hi");
                if (pack_send->destid != pack_send->sourceid)
                    send2shadow(pack_send);
            }
        }


    }


    


    return;


}

void printdata() {
    printf("k = %d:\n", simulation_slot);
    for (int i = 0;i < num_nodes;i++) {
        printf("node-%d:", i);
        for (int j = 0;j < num_nodes;j++) {
            printf("%d ", dts[i].costs[i][j]);
        }
        printf("\n");
    }

    return;
}

int route(int source, int dest, struct distance_table* dt) {
    return forwarding_table[source][dest];
}

void link_update(int packets) {
    for (int i = 0;i < MAX_NODES;i++) {
        if (visited[i] == -1)
            break;
        if (visited[i + 1] == -1)
            break;
        if (link_traffic[visited[i]][visited[i + 1]] == -1)
            break;
        link_traffic[visited[i]][visited[i + 1]] += packets;
        link_traffic[visited[i + 1]][visited[i]] += packets;
    }
    return;
    
}

void printlinkcosts() {
    for (int j = 0;j < num_nodes;j++) {
        for (int k = 0;k < num_nodes;k++) {
            printf(" %d ", link_costs[j][k]);
        }
        printf(" Node %d\n", j);
    }
}

void copy() {
    for (int k = 0; k < MAX_NODES;k++) {
        fl[k] = 0;
    }
    for (int j = 0;j < num_nodes;j++) {
        for (int k = 0;k < num_nodes;k++) {
            if (link_costs[j][k] != link_traffic[j][k] && link_traffic[j][k] != -1)
                fl[j] = 1;
            link_costs[j][k] = link_traffic[j][k];
        }
        
    }
    return;
}

void setlinktraffic() {
    for (int i = 0; i < num_nodes;i++) {
        for (int j = 0; j < num_nodes;j++) {
            if (link_costs[i][j] == -1)
                link_traffic[i][j] = -1;
            else
                link_traffic[i][j] = 0;
        }
    }
    return;
}

void printtraffic() {
    printf("k = %d:\n", simulation_slot);
    for (int i = 0;i < num_tra;i++) {
        for (int j = 0;j < 3;j++) {
            printf("%d ", traffic->traf[i][j]);
        }
        struct distance_table* dt = (struct distance_table*)malloc(sizeof(struct distance_table));
        dt->costs = (int**)malloc(num_nodes * sizeof(int*));
        for (int k = 0; k < num_nodes; k++) {
            dt->costs[k] = (int*)malloc(num_nodes * sizeof(int));
            if (dt->costs[k] == NULL) {
                printf("Error: Memory allocation failed.\n");
                exit(1);
            }

        }
        for (int m = 0;m < num_nodes;m++) {
            //printf("node-%d:", i);
            for (int n = 0;n < num_nodes;n++) {
                dt->costs[m][n] = dts[m].costs[m][n];
            }
            //printf("\n");
        }

        int distance = route(traffic->traf[i][0], traffic->traf[i][1], dt);
        /*for (int i = 0;i < MAX_NODES;i++) {
            if (visited[i] != -1) {
                printf(" %d ", visited[i]);
            }
        }
        printf("\n");*/
        link_update(traffic->traf[i][2]);
    }
    return;
}

void newrtupdate(struct distance_table* dt, int node) {
    
    //printf("\n");
    //printf("hi\n");
    //int i = node;

    int count = 0;
    int i, j;
   //rintdata();
    //intlinkcosts();
    
        // Find the minimum distance to each node from this node
        
    for (j = 0; j < num_nodes; j++) {
        int prev = dt->costs[node][j];
        int new = INT_MAX;
        //int min_distance = dt->costs[node][j];
        for (int i = 0;i < num_nodes;i++) {
            if (i != node && dt->costs[i][j] != -1 && link_costs[node][j] != -1) {
                
                if (new > dt->costs[i][j] + link_costs[node][i]) {
                    new = dt->costs[i][j] + link_costs[node][i];
                    forwarding_table[node][j] = i;
                }
                else
                    new = new;
            }
            if (link_costs[node][j] != -1 && node == i) {
               
                if (new > link_costs[node][j]) {
                    new = link_costs[node][j];
                    forwarding_table[node][j] = j;
                }
                else
                    new = new;
            }
        }
        //dt->costs[node][j] = new == INT_MAX ? link_costs[node][j] : new;
        if (new == INT_MAX && link_costs[node][j]!=-1) {
            dt->costs[node][j] = link_costs[node][j];
            new = link_costs[node][j];
            forwarding_table[node][j] = j;
        }
        else if (new!=INT_MAX && link_costs[node][j]!=-1) {
            dt->costs[node][j] = new;
            new = new;
        }
            
        
        if (dt->costs[node][j] != prev) {
            count++;
        }
        

    // Update the distance vector with the new minimum distances
    //->costs[node][j] = min_distance;
    }
    if ( count > 0) {
        for (int k = 0;k < num_nodes;k++) {
            if (dt->costs[node][k] != -1 && k != node) {
                struct rtpkt* pack_send = (struct rtpkt*)malloc(sizeof(struct rtpkt));
                pack_send->sourceid = node;
                pack_send->destid = k;
                pack_send->mincost = (int*)malloc(num_nodes * sizeof(int));
                for (int m = 0;m < num_nodes;m++) {
                    pack_send->mincost[m] = dt->costs[node][m];
                }
                //printf("hi");
                if (pack_send->destid != pack_send->sourceid)
                    send2shadow(pack_send);
            }
        }


    }


    
    return;
}

void main(int argc, char* argv[])
{
    struct event* eventptr;
    struct event* shadow;
    char* topology_file;
    char* traffic_file;
    FILE* fp;
    FILE* ft;
    char line[MAXLINE];
    char sentence[MAXLINE];
    int i, j;
    char* tok;
    char ch;
    int k_max = atoi(argv[1]);
    topology_file = argv[2];
    fp = fopen(topology_file, "r");
    traffic_file = argv[3];


    if (fgets(line, MAXLINE, fp) == NULL) {
        fprintf(stderr, "Error reading topology file\n");
        exit(1);
    }

    tok = strtok(line, " \t\n");
    while (tok != NULL) {
        num_nodes += 1;
        tok = strtok(NULL, " \t\n");
    }
    //printf("The number of nodes  are %d\n", num_nodes);
    fclose(fp);

    ft = fopen(traffic_file, "r");
    while ((ch = getc(ft)) != EOF) {
        //printf("%c\n",ch);
        if (ch == '\n')
            num_tra += 1;
    }

    fclose(ft);

    traffic = (struct traffic*)malloc(sizeof(struct traffic*));
    traffic->traf = (int**)malloc(num_tra * sizeof(int*));
    for (int i = 0; i < num_tra;i++) {
        traffic->traf[i] = (int*)malloc(3 * sizeof(int));
    }

    link_costs = (int**)malloc(num_nodes * sizeof(int*));
    for (int i = 0; i < num_nodes; i++)
    {
        link_costs[i] = (int*)malloc(num_nodes * sizeof(int));
    }

    forwarding_table = (int**)malloc(num_nodes * sizeof(int*));
    for (int i = 0; i < num_nodes; i++)
    {
        forwarding_table[i] = (int*)malloc(num_nodes * sizeof(int));
    }


    link_traffic = (int**)malloc(num_nodes * sizeof(int*));
    for (int i = 0; i < num_nodes; i++)
    {
        link_traffic[i] = (int*)malloc(num_nodes * sizeof(int));
    }

    fp = fopen(topology_file, "r");

    for (i = 0; i < num_nodes; i++) {
        if (fgets(line, MAXLINE, fp) == NULL) {
            fprintf(stderr, "Error reading topology file\n");
            exit(1);
        }
        tok = strtok(line, " \t\n");
        for (j = 0; j < num_nodes; j++) {
            link_costs[i][j] = atoi(tok);
            tok = strtok(NULL, " \t\n");
            //printf(" %d ", link_costs[i][j]);
        }
    }

    fclose(fp);
    //printf("The trafic num is %d\n", num_tra);
    ft = fopen(traffic_file, "r");
    for (i = 0; i < num_tra; i++) {
        if (fgets(sentence, MAXLINE, ft) == NULL) {
            fprintf(stderr, "Error reading the traffic file\n");
            exit(1);
        }
        tok = strtok(sentence, " \t\n");
        for (int j = 0;j < 3;j++) {
            traffic->traf[i][j] = atoi(tok);
            //printf("%d ", atoi(tok));
            tok = strtok(NULL, " \t\n");
        }
    }
    fclose(ft);

    //printf("%d\n",traffic->traf[0][0]);
    //printtraffic();
    simulation_slot++;
    //printf("File closed\n");

    dts = (struct distance_table*)calloc(num_nodes, sizeof(struct distance_table));
    if (dts == NULL) {
        printf("Error: Memory allocation failed.\n");
        exit(1);
    }
    for (int i = 0; i < num_nodes; i++) {
        for (int j = 0; j < num_nodes; j++) {
            forwarding_table[i][j] = -1;

            if (link_costs[i][j] != -1) {
                forwarding_table[i][j] = j;
            }
        }
    }
    for (int i = 0; i < num_nodes; i++)
    {
        rtinit(&dts[i], i, link_costs[i], num_nodes);
    }

    //printdata();

    /*for (int k = 0; k < num_nodes; k++)
    {
        for (int i = 0; i < num_nodes; i++) {
            for (int j = 0; j < num_nodes; j++) {
                printf("%d -- %d\n", k, dts[k].costs[i][j]);
            }
        }
    }*/

    while (1)
    {
        /* Todo: Please write the code here to handle the update of time slot k (We assume that in one slot k, the traffic can go through all the routers to reach the destination router)*/

        eventptr = evlist;            /* get next event to simulate */
        if (eventptr == NULL) {
            //printdata();
            mass:
            setlinktraffic();
            printf("k=%d:\n", simulation_slot);
            //printdata();
            for (int i = 0; i < num_tra; i++) {
                int traffic_source = traffic->traf[i][0];
                int traffic_destination = traffic->traf[i][1];
                int traffic_amount = traffic->traf[i][2];
                for (int k = 0;k < MAX_NODES;k++)
                    visited[k] = -1;
                printf("%d %d %d ", traffic_source, traffic_destination, traffic_amount);
                printf("%d>", traffic_source);
                visited[0] = traffic_source;
                int j = 1;
                int next_hop = route(traffic_source, traffic_destination, &dts[traffic_source]);
                while (next_hop != traffic_destination) {
                    printf("%d>", next_hop);
                    for (int m = 0;m < MAX_NODES;m++) {
                        if (next_hop == visited[m]) {
                            printf("\b(drop)\n");
                            goto mama;
                        }
                            //goto mama;
                    }
                    visited[j] = next_hop;
                    next_hop = route(next_hop, traffic_destination, &dts[next_hop]);
                    j++;
                }
                printf("%d\n", traffic_destination);
                visited[j] = traffic_destination;
                link_update(traffic_amount);

            mama:
                ;//
            }

            copy();
            
            
            //printlinkcosts();
            
            simulation_slot++;
            evlist = shadowlist;
            eventptr = shadowlist;
            shadowlist = NULL;
            if (simulation_slot > k_max)
                goto terminate;
            for (int n = 0;n < num_nodes;n++) {
                if (fl[n] == 1)
                    newrtupdate(&dts[n], n);
            }

           
            //intdata();
            if (eventptr == NULL) {
                if (simulation_slot <= k_max)
                    goto mass;
                goto terminate;
            }
                
        }

        /*for (int i = 0; i < num_nodes;i++) {
            for (int j = 0; j < num_nodes;j++) {
                printf("%d ", dts[2].costs[i][j]);
            }
            printf("\n");
        }
        printf("\n");*/
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        clocktime = eventptr->evtime;    /* update time to next event time */
        struct rtpkt* pk = eventptr->rtpktptr;
        //("%p\n", eventptr->rtpktptr);
        //for (int j = 0; j < num_nodes; j++) {
         //   printf("%d  ", pk->mincost[j]);
        //}
        if (eventptr->evtype == FROM_LAYER2)
        {
            /* Todo: You need to modify the rtupdate method and add more codes here for Part B and Part C, since the link costs in these parts are dynamic.*/
            
            rtupdate(&dts[eventptr->eventity], *(eventptr->rtpktptr)); 
            
        }
        else
        {
            printf("Panic: unknown event type\n"); exit(0);
        }
        if (eventptr->evtype == FROM_LAYER2)
            free(eventptr->rtpktptr);        /* free memory for packet, if any */
        free(eventptr);                    /* free memory for event struct   */
    }


terminate:
    ;
}



/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
float jimsrand()
{
    double mmm = 2147483647;
    float x;
    x = rand() / mmm;
    return(x);
}



void insertevent(struct event* p)
{
    struct event* q, * qold;

    q = evlist;     /* q points to header of list in which p struct inserted */
    if (q == NULL) {   /* list is empty */
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL) {   /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist) { /* front of list */
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else {     /* middle of list */
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist()
{
    struct event* q;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next) {
        printf("Event time: %f, type: %d entity: %d Source: %d\n", q->evtime, q->evtype, q->eventity, (q->rtpktptr)->sourceid);
    }
    printf("--------------\n");
}


/************************** send update to neighbor (packet.destid)***************/
void send2neighbor(struct rtpkt* packet)
{
    struct event* evptr, * q;
    float jimsrand(), lastime;
    int i;

    /* be nice: check if source and destination id's are reasonable */
    if (packet->sourceid<0 || packet->sourceid >num_nodes) {
        printf("WARNING: illegal source id in your packet, ignoring packet!\n");
        return;
    }
    if (packet->destid<0 || packet->destid > num_nodes) {
        printf("WARNING: illegal dest id in your packet, ignoring packet!\n");
        return;
    }
    if (packet->sourceid == packet->destid) {
        printf("WARNING: source and destination id's the same, ignoring packet!\n");
        return;
    }


    /* create future event for arrival of packet at the other side */
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER2;   /* packet will pop out from layer3 */
    evptr->eventity = packet->destid; /* event occurs at other entity */
    evptr->rtpktptr = packet;       /* save ptr to my copy of packet */
    struct rtpkt* pka = evptr->rtpktptr;
    //for (int j = 0; j < num_nodes; j++) {
    //    printf("%d  ", pka->mincost[j]);
    //}
    //printf("dist %p  ", packet);
    /* finally, compute the arrival time of packet at the other end.
       medium can not reorder, so make sure packet arrives between 1 and 10
       time units after the latest arrival time of packets
       currently in the medium on their way to the destination */
    lastime = clocktime;
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER2 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 2. * jimsrand();
    insertevent(evptr);
    //printevlist();
    //printf("Packet Sent\n");
}

void inserteventshadow(struct event* p)
{
    struct event* q, * qold;

    q = shadowlist;     /* q points to header of list in which p struct inserted */
    if (q == NULL) {   /* list is empty */
        shadowlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL) {   /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == shadowlist) { /* front of list */
            p->next = shadowlist;
            p->prev = NULL;
            p->next->prev = p;
            shadowlist = p;
        }
        else {     /* middle of list */
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void send2shadow(struct rtpkt* packet)
{
    struct event* evptr, * q;
    float jimsrand(), lastime;
    int i;

    /* be nice: check if source and destination id's are reasonable */
    if (packet->sourceid<0 || packet->sourceid >num_nodes) {
        printf("WARNING: illegal source id in your packet, ignoring packet!\n");
        return;
    }
    if (packet->destid<0 || packet->destid > num_nodes) {
        printf("WARNING: illegal dest id in your packet, ignoring packet!\n");
        return;
    }
    if (packet->sourceid == packet->destid) {
        printf("WARNING: source and destination id's the same, ignoring packet!\n");
        return;
    }


    /* create future event for arrival of packet at the other side */
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER2;   /* packet will pop out from layer3 */
    evptr->eventity = packet->destid; /* event occurs at other entity */
    evptr->rtpktptr = packet;       /* save ptr to my copy of packet */
    struct rtpkt* pka = evptr->rtpktptr;
    //for (int j = 0; j < num_nodes; j++) {
    //    printf("%d  ", pka->mincost[j]);
    //}
    //printf("dist %p  ", packet);
    /* finally, compute the arrival time of packet at the other end.
       medium can not reorder, so make sure packet arrives between 1 and 10
       time units after the latest arrival time of packets
       currently in the medium on their way to the destination */
    lastime = clocktime;
    for (q = shadowlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER2 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 2. * jimsrand();
    inserteventshadow(evptr);
    //printevlist();
    //printf("Packet Sent\n");
}
