/**************************************************
 * Created: 2013/4/11
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/11 22:00
 * Version: 0.1
 * File: src/masternode/request.c
 * Breif: Source file for request handling mechanism.
 **************************************************/

#include <mpi.h>

#include "request.h"
#include "common.h"



/* MPI对应的struct request类型 */
void
build_mpi_type_request(struct request *input, MPI_Datatype *mpitype)
{
    int block_lengths[2];
    MPI_Aint displacements[2];
    MPI_Aint addresses[3];
    MPI_Datatype typelist[2];

    typelist[0] = MPI_INT;
    typelist[1] = MPI_INT;

    block_lengths[0] = block_lengths[1] = 1;

    MPI_Get_address(input, &addresses[0]);
    MPI_Get_address(&input->request, &addresses[1]);
    MPI_Get_address(&input->tag, &addresses[2]);
    displacements[0] = addresses[1] - addresses[0];
    displacements[1] = addresses[2] - addresses[1];

    MPI_Type_struct(2, block_lengths, displacements, typelist, mpitype);

    MPI_Type_commit(mpitype);
}

/* MPI对应的struct slave_info_req类型 */
void
build_mpi_type_slave_info(struct slave_info *input, MPI_Datatype *mpitype)
{
    int block_lengths[2];
    MPI_Aint displacements[2];
    MPI_Aint addresses[3];
    MPI_Datatype typelist[2];

    typelist[0] = MPI_INT;
    typelist[1] = MPI_UNSIGNED_LONG;
    block_lengths[0] = 2;
    block_lengths[1] = 3;

    MPI_Get_address(input, &addresses[0]);
    MPI_Get_address(&input->id, &addresses[1]);
    MPI_Get_address(&input->free, &addresses[2]);
    displacements[0] = addresses[1] - addresses[0];
    displacements[1] = addresses[2] - addresses[1];
        
    MPI_Type_struct(2, block_lengths, displacements, typelist, mpitype);

    MPI_Type_commit(mpitype);
}

/* MPI对应的struct block_req类型 */
void
build_mpi_type_block(struct block *input, MPI_Datatype *mpitype)
{
    int block_lengths[2];
    MPI_Aint displacements[2];
    MPI_Aint addresses[3];
    MPI_Datatype typelist[2];

    typelist[0] = MPI_INT;
    typelist[1] = MPI_UNSIGNED_LONG;
    block_lengths[0] = 2;
    block_lengths[1] = 2;

    MPI_Get_address(input, &addresses[0]);
    MPI_Get_address(&input->slave_id, &addresses[1]);
    MPI_Get_address(&input->size, &addresses[2]);
    displacements[0] = addresses[1] - addresses[0];
    displacements[1] = addresses[2] - addresses[1];
        
    MPI_Type_struct(2, block_lengths, displacements, typelist, mpitype);

    MPI_Type_commit(mpitype);
}

/* MPI对应的struct share_file_req类型 */
void
build_mpi_type_share_file(struct share_file *input, MPI_Datatype *mpitype)
{
    int block_lengths[4];
    MPI_Aint displacements[4];
    MPI_Aint addresses[5];
    MPI_Datatype typelist[4];

    MPI_Datatype mpi_block_type;
    struct block block;
    build_mpi_type_block(&block, &mpi_block_type);
    
    
    typelist[0] = MPI_CHAR;
    typelist[1] = MPI_UNSIGNED_LONG;
    typelist[2] = mpi_block_type;
    typelist[3] = MPI_INT;
    block_lengths[0] = MAX_PATH_LENGTH;
    block_lengths[1] = 1;
    block_lengths[2] = MAX_BLOCKS;
    block_lengths[3] = 1;

    MPI_Get_address(input, &addresses[0]);
    MPI_Get_address(&input->hnode.str, &addresses[1]);
    MPI_Get_address(&input->size, &addresses[2]);
    MPI_Get_address(&input->blocks, &addresses[3]);
    MPI_Get_address(&input->block_num, &addresses[4]);
    displacements[0] = addresses[1] - addresses[0];
    displacements[1] = addresses[2] - addresses[1];
    displacements[2] = addresses[3] - addresses[2];
    displacements[3] = addresses[4] - addresses[3];
        
    MPI_Type_struct(4, block_lengths, displacements, typelist, mpitype);

    MPI_Type_commit(mpitype);
}
