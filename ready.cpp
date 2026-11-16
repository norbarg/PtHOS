#include <mpi.h>
#include <cstdio>
#include <vector>

int main(int argc, char* argv[])
{
    int rank, size;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0)
    {
        std::vector<int> in(size);
        std::vector<MPI_Request> req(size - 1);

        for (int src = 1; src < size; src++)
        {
            MPI_Irecv(
                &in[src],
                1,
                MPI_INT,
                src,
                0,
                MPI_COMM_WORLD,
                &req[src - 1]
            );
        }

        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Waitall(
            size - 1,
            req.data(),
            MPI_STATUSES_IGNORE
        );

        for (int src = 1; src < size; src++)
        {
            printf("from %d: %d\n", src, in[src]);
        }
    }
    else
    {
        int value = rank * 10;

        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Rsend(
            &value,
            1,
            MPI_INT,
            0,
            0,
            MPI_COMM_WORLD
        );
    }

    MPI_Finalize();
    return 0;
}