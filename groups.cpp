#include <mpi.h>
#include <cstdio>

int main(int argc, char* argv[])
{
    // Наш вариант
    int r0[] = {5, 2, 1, 7}; // P1: a, e, d, h
    int r1[] = {6, 0, 3, 4}; // P2: b, c, f, g

    int rank, size, lrank;

    MPI_Group world_gr, gr0, gr1;
    MPI_Comm comm0, comm1, local, inter;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 8)
    {
        if (rank == 0)
        {
            printf("Program requires exactly 8 processes.\n");
        }

        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_group(
        MPI_COMM_WORLD,
        &world_gr
    );

    MPI_Group_incl(
        world_gr,
        4,
        r0,
        &gr0
    );

    MPI_Group_incl(
        world_gr,
        4,
        r1,
        &gr1
    );

    // Колективні виклики — виконують усі процеси
    MPI_Comm_create(
        MPI_COMM_WORLD,
        gr0,
        &comm0
    );

    MPI_Comm_create(
        MPI_COMM_WORLD,
        gr1,
        &comm1
    );

    bool inG0 = (comm0 != MPI_COMM_NULL);

    local = inG0 ? comm0 : comm1;

    int remoteLeader =
        inG0 ? r1[0] : r0[0];

    MPI_Intercomm_create(
        local,
        0,
        MPI_COMM_WORLD,
        remoteLeader,
        99,
        &inter
    );

    MPI_Comm_rank(
        local,
        &lrank
    );

    // Процес з локальним рангом k
    // обмінюється з процесом k іншої групи

    int out = rank;
    int in = -1;

    MPI_Sendrecv(
        &out,
        1,
        MPI_INT,
        lrank,
        0,

        &in,
        1,
        MPI_INT,
        lrank,
        0,

        inter,
        MPI_STATUS_IGNORE
    );

    printf(
        "world %d: group r%d, local %d, partner %d\n",
        rank,
        inG0 ? 0 : 1,
        lrank,
        in
    );

    MPI_Comm_free(&inter);
    MPI_Comm_free(&local);

    MPI_Group_free(&gr0);
    MPI_Group_free(&gr1);
    MPI_Group_free(&world_gr);

    MPI_Finalize();

    return 0;
}