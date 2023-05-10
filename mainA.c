#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma warning(disable : 4996)
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



/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 2 and below network environment:
  - emulates the transmission and delivery (with no loss and no
    corruption) between two physically connected nodes
  - calls the initializations routine rtinit once before
    beginning emulation for each node.

You should read and understand the code below. For Part A, you should fill all parts with annotation starting with "Todo". For Part B and Part C, you need to add additional routines for their features.
******************************************************************/

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
struct distance_table* dts;
int simulation_slot = 0;
int** link_costs; /*This is a 2D matrix stroing the content defined in topo file*/
int num_nodes;
#define MAXLINE 1024
/* possible events: */
/*Note in this lab, we only have one event, namely FROM_LAYER2.It refer to that the packet will pop out from layer3, you can add more event to emulate other activity for other layers. Like FROM_LAYER3*/
#define  FROM_LAYER2     1

float clocktime = 0.000;
void send2neighbor(struct rtpkt*);
void send2shadow(struct rtpkt*);
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
    /* Todo: Please write the code here*/
    /*Recieves the dt of the desination and the packet received from a source*/
    //printf("%d\n", recv_pkt.sourceid);
    
    //printf("REcv\n");
    
   
    //printf("\n");
    //printf("hi\n");

    for (int i = 0;i < num_nodes;i++) {
        int count = 0;
        for (int j = 0;j < num_nodes;j++) {
            if (dt->costs[i][j] == -1 && recv_pkt.mincost[j] != -1 && dt->costs[i][recv_pkt.sourceid] != -1) {
                dt->costs[i][j] = dt->costs[i][recv_pkt.sourceid] + recv_pkt.mincost[j];
                count++;
            }
            if ( dt->costs[i][j] > recv_pkt.mincost[j] + dt->costs[i][recv_pkt.sourceid] && recv_pkt.mincost[j] != -1 && dt->costs[i][recv_pkt.sourceid]!=-1) {
                dt->costs[i][j] = dt->costs[i][recv_pkt.sourceid] + recv_pkt.mincost[j];
                count++;
            }
            
            if (i == recv_pkt.sourceid) {
                dt->costs[i][j] = recv_pkt.mincost[j];
            }
        if (i == recv_pkt.destid && count>0) {
            for (int k = 0;k < num_nodes;k++) {
                if (dt->costs[i][k] != -1 && k!=i) {
                    struct rtpkt* pack_send = (struct rtpkt*)malloc(sizeof(struct rtpkt));
                    pack_send->sourceid = recv_pkt.destid;
                    pack_send->destid = k;
                    pack_send->mincost = (int*)malloc(num_nodes * sizeof(int));
                    for (int m = 0;m < num_nodes;m++) {
                        pack_send->mincost[m] = dt->costs[i][m];
                    }
                    //printf("hi");
                    send2shadow(pack_send);
                }
            }
            
            
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

void main(int argc, char* argv[])
{
    struct event* eventptr;
    struct event* shadow;
    char* topology_file;
    FILE* fp;
    char line[MAXLINE];
    int i, j;
    char* tok;
    /* Todo: Please write the code here to process the input.
    Given different flag, you have different number of input for part A, B, C.
    Please write your own code to parse the input for each part.
    Specifically, in part A you need parse the input file and get “num_nodes”,
    and fill in the content of dts and link_costs */
    int k_max = atoi(argv[1]);
    topology_file = argv[2];
    fp = fopen(topology_file, "r");

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

    link_costs = (int**)malloc(num_nodes * sizeof(int*));
    for (int i = 0; i < num_nodes; i++)
    {
        link_costs[i] = (int*)malloc(num_nodes * sizeof(int));
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

    //printf("File closed\n");

    dts = (struct distance_table*)calloc(num_nodes, sizeof(struct distance_table));
    if (dts == NULL) {
        printf("Error: Memory allocation failed.\n");
        exit(1);
    }

    for (int i = 0; i < num_nodes; i++)
    {
        rtinit(&dts[i], i, link_costs[i], num_nodes);
    }

    printdata();
    simulation_slot++;
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
            macho:
            if ((simulation_slot <= 4 || simulation_slot % 10 == 0) )
                printdata();
            if (!(simulation_slot <= k_max)) {
                goto terminate;
            }
            simulation_slot++;
            evlist = shadowlist;
            eventptr = shadowlist;
            shadowlist = NULL;
            if (eventptr == NULL)
                goto macho;
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
