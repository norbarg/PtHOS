#include <mpi.h>
#include <cstdio>
#include <cstddef>

// ======================================================
// Дані нашого варіанта
// ======================================================

const int NV = 8;

// Групи у номерах вершин
int r0[4] = {6, 3, 2, 8}; // P1: a, e, d, h
int r1[4] = {7, 1, 4, 5}; // P2: b, c, f, g

// Ваги вершин.
// Індекс 0 не використовується.
int weight[NV + 1] =
{
    0,
    2, // vertex 1
    4, // vertex 2
    5, // vertex 3
    3, // vertex 4
    2, // vertex 5
    2, // vertex 6
    3, // vertex 7
    1  // vertex 8
};

// Дуги графа: {відправник, одержувач}
int edges[][2] =
{
    {6, 2},
    {7, 2},
    {6, 3},
    {6, 1},
    {7, 5},
    {3, 8},
    {1, 4},
    {1, 5},
    {4, 8},
    {5, 8},
    {2, 8}
};

const int NE = sizeof(edges) / sizeof(edges[0]);

// ======================================================
// Тип повідомлення
// ======================================================

struct Msg
{
    int from;
    int to;
    double tSend;
    int value;
};

MPI_Datatype MSG_TYPE;

// ======================================================
// MPI-групи і комунікатори
// ======================================================

MPI_Group world_gr;
MPI_Group gr_0;
MPI_Group gr_1;
MPI_Group my_gr;
MPI_Group other_gr;

MPI_Comm comm_0;
MPI_Comm comm_1;
MPI_Comm my_comm;
MPI_Comm inter_comm;

FILE* logf;
double t0;

// ======================================================
// Створення типу повідомлення
// ======================================================

void createMsgType()
{
    int len[4] = {1, 1, 1, 1};

    MPI_Aint disp[4] =
    {
        offsetof(Msg, from),
        offsetof(Msg, to),
        offsetof(Msg, tSend),
        offsetof(Msg, value)
    };

    MPI_Datatype types[4] =
    {
        MPI_INT,
        MPI_INT,
        MPI_DOUBLE,
        MPI_INT
    };

    MPI_Type_create_struct(
        4,
        len,
        disp,
        types,
        &MSG_TYPE
    );

    MPI_Type_commit(&MSG_TYPE);
}

// ======================================================
// Створення груп і комунікаторів
// ======================================================

void createComms()
{
    int g0[4];
    int g1[4];

    for (int i = 0; i < 4; i++)
    {
        // номер вершини -> глобальний MPI-ранг
        g0[i] = r0[i] - 1;
        g1[i] = r1[i] - 1;
    }

    MPI_Comm_group(
        MPI_COMM_WORLD,
        &world_gr
    );

    MPI_Group_incl(
        world_gr,
        4,
        g0,
        &gr_0
    );

    MPI_Group_incl(
        world_gr,
        4,
        g1,
        &gr_1
    );

    MPI_Comm_create(
        MPI_COMM_WORLD,
        gr_0,
        &comm_0
    );

    MPI_Comm_create(
        MPI_COMM_WORLD,
        gr_1,
        &comm_1
    );

    bool inG0 = (comm_0 != MPI_COMM_NULL);

    my_comm = inG0 ? comm_0 : comm_1;
    my_gr = inG0 ? gr_0 : gr_1;
    other_gr = inG0 ? gr_1 : gr_0;

    int remote_leader =
        inG0 ? g1[0] : g0[0];

    MPI_Intercomm_create(
        my_comm,
        0,
        MPI_COMM_WORLD,
        remote_leader,
        2809,
        &inter_comm
    );
}

// ======================================================
// Вибір комунікатора
// ======================================================

MPI_Comm commFor(int partner, int* prank)
{
    MPI_Group_translate_ranks(
        world_gr,
        1,
        &partner,
        my_gr,
        prank
    );

    if (*prank != MPI_UNDEFINED)
    {
        return my_comm;
    }

    MPI_Group_translate_ranks(
        world_gr,
        1,
        &partner,
        other_gr,
        prank
    );

    return inter_comm;
}

// ======================================================
// Вершинний процес
// ======================================================

void runVertex(int v)
{
    Msg m;
    MPI_Status st;
    int pr;

    // Прийом від усіх попередників
    for (int e = 0; e < NE; e++)
    {
        if (edges[e][1] != v)
        {
            continue;
        }

        int src = edges[e][0];

        MPI_Comm c =
            commFor(src - 1, &pr);

        MPI_Recv(
            &m,
            1,
            MSG_TYPE,
            pr,
            src,
            c,
            &st
        );

        fprintf(
            logf,
            "%7.3f recv %d <- %d, value %d\n",
            MPI_Wtime() - t0,
            v,
            m.from,
            m.value
        );
    }

    fprintf(
        logf,
        "%7.3f start %d\n",
        MPI_Wtime() - t0,
        v
    );

    // Імітація виконання процесу
    double tb = MPI_Wtime();

    while (MPI_Wtime() - tb < weight[v])
    {
    }

    // Передача всім наступникам
    for (int e = 0; e < NE; e++)
    {
        if (edges[e][0] != v)
        {
            continue;
        }

        int dst = edges[e][1];

        m.from = v;
        m.to = dst;
        m.value = v * 10 + dst;
        m.tSend = MPI_Wtime() - t0;

        MPI_Comm c =
            commFor(dst - 1, &pr);

        MPI_Send(
            &m,
            1,
            MSG_TYPE,
            pr,
            v,
            c
        );

        fprintf(
            logf,
            "%7.3f send %d -> %d, value %d\n",
            m.tSend,
            v,
            dst,
            m.value
        );
    }

    fprintf(
        logf,
        "%7.3f end %d\n",
        MPI_Wtime() - t0,
        v
    );
}

// ======================================================
// main
// ======================================================

int main(int argc, char* argv[])
{
    int rank;
    int size;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(
        MPI_COMM_WORLD,
        &rank
    );

    MPI_Comm_size(
        MPI_COMM_WORLD,
        &size
    );

    if (size != 8)
    {
        if (rank == 0)
        {
            printf(
                "Program requires exactly 8 processes.\n"
            );
        }

        MPI_Abort(
            MPI_COMM_WORLD,
            1
        );
    }

    createComms();
    createMsgType();

    // Створюємо log1.txt ... log8.txt
    char filename[32];

    sprintf_s(
        filename,
        "log%d.txt",
        rank + 1
    );

    fopen_s(
        &logf,
        filename,
        "w"
    );

    // Перевірка груп і комунікаторів
    int lrank;
    int lsize;
    int rsize;
    int isInter;

    MPI_Comm_rank(
        my_comm,
        &lrank
    );

    MPI_Comm_size(
        my_comm,
        &lsize
    );

    MPI_Comm_remote_size(
        inter_comm,
        &rsize
    );

    MPI_Comm_test_inter(
        inter_comm,
        &isInter
    );

    fprintf(
        logf,
        "vertex %d: local rank %d, "
        "local size %d, remote size %d, inter %d\n",
        rank + 1,
        lrank,
        lsize,
        rsize,
        isInter
    );

    // Одночасний старт усіх процесів
    MPI_Barrier(MPI_COMM_WORLD);

    t0 = MPI_Wtime();

    runVertex(rank + 1);

    fclose(logf);

    MPI_Type_free(&MSG_TYPE);

    MPI_Comm_free(&inter_comm);
    MPI_Comm_free(&my_comm);

    MPI_Group_free(&gr_0);
    MPI_Group_free(&gr_1);
    MPI_Group_free(&world_gr);

    MPI_Finalize();

    return 0;
}