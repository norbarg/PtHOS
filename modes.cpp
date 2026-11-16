#include <mpi.h>
#include <cstdio>
#include <vector>
#include <windows.h>

int main(int argc, char* argv[])
{
    const int N = 100000;

    std::vector<int> data(N);
    int rank;

    setvbuf(stdout, NULL, _IONBF, 0);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
    {
        int bsize = static_cast<int>(N * sizeof(int)) + MPI_BSEND_OVERHEAD;
        char* buf = new char[bsize];

        MPI_Buffer_attach(buf, bsize);

        double t = MPI_Wtime();

        MPI_Send(
            data.data(),
            N,
            MPI_INT,
            1,
            1,
            MPI_COMM_WORLD
        );

        printf("MPI_Send : %.2f s\n", MPI_Wtime() - t);

        t = MPI_Wtime();

        MPI_Ssend(
            data.data(),
            N,
            MPI_INT,
            1,
            2,
            MPI_COMM_WORLD
        );

        printf("MPI_Ssend: %.2f s\n", MPI_Wtime() - t);

        t = MPI_Wtime();

        MPI_Bsend(
            data.data(),
            N,
            MPI_INT,
            1,
            3,
            MPI_COMM_WORLD
        );

        printf("MPI_Bsend: %.2f s\n", MPI_Wtime() - t);

        MPI_Buffer_detach(&buf, &bsize);
        delete[] buf;
    }
    else if (rank == 1)
    {
        MPI_Status st;

        for (int tag = 1; tag <= 3; tag++)
        {
            Sleep(2000);

            MPI_Recv(
                data.data(),
                N,
                MPI_INT,
                0,
                tag,
                MPI_COMM_WORLD,
                &st
            );
        }
    }

    MPI_Finalize();
    return 0;
}