#include "libISAnalysisAdaptor.h"

#include "libISSchema.h"
#include "DataAdaptor.h"
#include "MeshMetadataMap.h"
#include "VTKUtils.h"
#include "MPIUtils.h"
#include "Timer.h"
#include "Error.h"

#include <vtkCellTypes.h>
#include <vtkCellData.h>
#include <vtkCompositeDataIterator.h>
#include <vtkCompositeDataSet.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkLongArray.h>
#include <vtkUnsignedLongArray.h>
#include <vtkCharArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkIdTypeArray.h>
#include <vtkCellArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPolyData.h>
#include <vtkImageData.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>

#include <mpi.h>
#include <adios.h>
#include <vector>

namespace sensei
{

//----------------------------------------------------------------------------
senseiNewMacro(libISAnalysisAdaptor);

//----------------------------------------------------------------------------
libISAnalysisAdaptor::libISAnalysisAdaptor() : MaxBufferSize(500),
    Schema(nullptr), Method("MPI"), FileName("sensei.bp"), GroupHandle(0)
{
}

//----------------------------------------------------------------------------
libISAnalysisAdaptor::~libISAnalysisAdaptor()
{
  delete this->Schema;
}

//-----------------------------------------------------------------------------
int libISAnalysisAdaptor::SetDataRequirements(const DataRequirements &reqs)
{
  this->Requirements = reqs;
  return 0;
}

//-----------------------------------------------------------------------------
int libISAnalysisAdaptor::AddDataRequirement(const std::string &meshName,
  int association, const std::vector<std::string> &arrays)
{
  this->Requirements.AddRequirement(meshName, association, arrays);
  return 0;
}

//----------------------------------------------------------------------------
bool libISAnalysisAdaptor::Execute(DataAdaptor* dataAdaptor)
{
  timer::MarkEvent mark("libISAnalysisAdaptor::Execute");

  // figure out what the simulation can provide
  MeshMetadataFlags flags;
  flags.SetBlockDecomp();
  flags.SetBlockSize();

  MeshMetadataMap mdm;
  if (mdm.Initialize(dataAdaptor, flags))
    {
    SENSEI_ERROR("Failed to get metadata")
    return false;
    }

  // if no dataAdaptor requirements are given, push all the data
  // fill in the requirements with every thing
  if (this->Requirements.Empty())
    {
    if (this->Requirements.Initialize(dataAdaptor, false))
      {
      SENSEI_ERROR("Failed to initialze dataAdaptor description")
      return false;
      }
    SENSEI_WARNING("No subset specified. Writing all available data")
    }

  // collect the specified data objects and metadata
  std::vector<vtkCompositeDataSet*> objects;
  std::vector<MeshMetadataPtr> metadata;

  MeshRequirementsIterator mit =
    this->Requirements.GetMeshRequirementsIterator();

  while (mit)
    {
    // get metadata
    MeshMetadataPtr md;
    if (mdm.GetMeshMetadata(mit.MeshName(), md))
      {
      SENSEI_ERROR("Failed to get mesh metadata for mesh \""
        << mit.MeshName() << "\"")
      return false;
      }

    // get the mesh
    vtkCompositeDataSet *dobj = nullptr;
    if (dataAdaptor->GetMesh(mit.MeshName(), mit.StructureOnly(), dobj))
      {
      SENSEI_ERROR("Failed to get mesh \"" << mit.MeshName() << "\"")
      return false;
      }

    // add the ghost cell arrays to the mesh
    if (md->NumGhostCells && dataAdaptor->AddGhostCellsArray(dobj, mit.MeshName()))
      {
      SENSEI_ERROR("Failed to get ghost cells for mesh \"" << mit.MeshName() << "\"")
      return false;
      }

    // add the ghost node arrays to the mesh
    if (md->NumGhostNodes && dataAdaptor->AddGhostNodesArray(dobj, mit.MeshName()))
      {
      SENSEI_ERROR("Failed to get ghost nodes for mesh \"" << mit.MeshName() << "\"")
      return false;
      }

    // add the required arrays
    ArrayRequirementsIterator ait =
      this->Requirements.GetArrayRequirementsIterator(mit.MeshName());

    while (ait)
      {
      if (dataAdaptor->AddArray(dobj, mit.MeshName(),
         ait.Association(), ait.Array()))
        {
        SENSEI_ERROR("Failed to add "
          << VTKUtils::GetAttributesName(ait.Association())
          << " data array \"" << ait.Array() << "\" to mesh \""
          << mit.MeshName() << "\"")
        return false;
        }

      ++ait;
      }

    // generate a global view of the metadata. everything we do from here
    // on out depends on having the global view.
    if (!md->GlobalView)
      {
      MPI_Comm comm = this->GetCommunicator();
      sensei::MPIUtils::GlobalViewV(comm, md->BlockOwner);
      sensei::MPIUtils::GlobalViewV(comm, md->BlockIds);
      sensei::MPIUtils::GlobalViewV(comm, md->BlockNumPoints);
      sensei::MPIUtils::GlobalViewV(comm, md->BlockNumCells);
      sensei::MPIUtils::GlobalViewV(comm, md->BlockCellArraySize);
      sensei::MPIUtils::GlobalViewV(comm, md->BlockExtents);
      md->GlobalView = true;
      }

    // add to the collection
    objects.push_back(dobj);
    metadata.push_back(md);

    ++mit;
    }

  unsigned long timeStep = dataAdaptor->GetDataTimeStep();
  double time = dataAdaptor->GetDataTime();

  if (this->InitializelibIS(metadata) ||
    this->WriteTimestep(timeStep, time, metadata, objects))
    return false;

  unsigned int n_objects = objects.size();
  for (unsigned int i = 0; i < n_objects; ++i)
    objects[i]->Delete();

  return true;
}

//----------------------------------------------------------------------------
int libISAnalysisAdaptor::InitializelibIS(
  const std::vector<MeshMetadataPtr> &metadata)
{
  timer::MarkEvent mark("libISAnalysisAdaptor::IntializelibIS");

  if (!this->Schema)
    {
    // initialize adios
    adios_init_noxml(this->GetCommunicator());

#if ADIOS_VERSION_GE(1,11,0)
    adios_set_max_buffer_size(this->MaxBufferSize);
    adios_declare_group(&this->GroupHandle, "SENSEI", "",
      static_cast<ADIOS_STATISTICS_FLAG>(adios_flag_no));
#else
    adios_allocate_buffer(ADIOS_BUFFER_ALLOC_NOW, this->MaxBufferSize);
    adios_declare_group(&this->GroupHandle, "SENSEI", "", adios_flag_no);
#endif

    adios_select_method(this->GroupHandle, this->Method.c_str(), "", "");

    // define libIS variables
    this->Schema = new senseilibIS::DataObjectCollectionSchema;
    }

  // (re)define variables to support meshes that evovle in time
  if (this->Schema->DefineVariables(this->GetCommunicator(),
    this->GroupHandle, metadata))
    {
    SENSEI_ERROR("Failed to define variables")
    return -1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int libISAnalysisAdaptor::FinalizelibIS()
{
  int rank = 0;
  MPI_Comm_rank(this->GetCommunicator(), &rank);
  adios_finalize(rank);
  return 0;
}

//----------------------------------------------------------------------------
int libISAnalysisAdaptor::Finalize()
{
  timer::MarkEvent mark("libISAnalysisAdaptor::Finalize");

  if (this->Schema)
    this->FinalizelibIS();

  delete this->Schema;
  this->Schema = nullptr;

  return 0;
}

//----------------------------------------------------------------------------
int libISAnalysisAdaptor::WriteTimestep(unsigned long timeStep,
  double time, const std::vector<MeshMetadataPtr> &metadata,
  const std::vector<vtkCompositeDataSet*> &objects)
{
  timer::MarkEvent mark("libISAnalysisAdaptor::WriteTimestep");

  int ierr = 0;
  int64_t handle = 0;

  adios_open(&handle, "sensei", this->FileName.c_str(),
    timeStep == 0 ? "w" : "a", this->GetCommunicator());

  // TODO -- what are the implications of not setting the group
  // size? it's a lot of work to calculate the size. user manual
  // indicates that it is optional. would setting a fixed size
  // like 200MB be better than not setting the size?
  //
  /*uint64_t group_size = this->Schema->GetSize(
    this->GetCommunicator(), metadata, objects);
  adios_group_size(handle, group_size, &group_size);*/

  if (this->Schema->Write(this->GetCommunicator(),
    handle, timeStep, time, metadata, objects))
    {
    SENSEI_ERROR("Failed to write step " << timeStep
      << " to \"" << this->FileName << "\"")
    ierr = -1;
    }

  adios_close(handle);

  return ierr;
}

//----------------------------------------------------------------------------
void libISAnalysisAdaptor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

}