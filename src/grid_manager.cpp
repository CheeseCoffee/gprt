#include "grid_manager.h"
#include "grid.h"
#include "cell.h"
#include "config.h"
#include "solver.h"
#include "out_results.h"
#include "vessel_grid.h"

GridManager::GridManager() :
m_pGrid(new Grid),
m_pOutResults(new OutResults()),
m_pSolver(nullptr) {}

void GridManager::Init() {
  m_pGrid->Init();
  m_pGrid->SetParent(this);
  m_pOutResults->Init(m_pGrid.get(), this);
}

void GridManager::SetParent(Solver* pSolver) {
  m_pSolver = pSolver;
}

Config* GridManager::GetConfig() const {
  return m_pSolver->GetConfig();
}

OutResults* GridManager::GetOutResults() const {
  return m_pOutResults.get();
}

void GridManager::BuildGrid() {
  Config* pConfig = GetConfig();
  Build(pConfig);
  FillInGrid(pConfig);
  
  if (pConfig->GetUseVessels())
    InitVessels();

  LinkCells(pConfig);
}

// Currently we don't need this func
void GridManager::SaveGridConfig(Config* pConfig) {
  std::string file_name;
  try {
    file_name = GenerateFileName(pConfig->GetGridGeometryType());
  } catch (char const* sExc) {
    std::cout << "Exception occurs: " << sExc << std::endl;
    return;
  }
  Write(file_name);
}

void GridManager::LoadGridConfig(Config* pConfig) {
  std::string file_name;
  try {
    file_name = GenerateFileName(pConfig->GetGridGeometryType());
  } catch (char const* sExc) {
    std::cout << "Exception occurs: " << sExc << std::endl;
    return;
  }
  Read(file_name);
}

bool GridManager::Write(const std::string& sName) {
  // TODO: To implement
  return true;
}

bool GridManager::Read(const std::string& sName) {
  // TODO: To implement
  return true;
}

const std::string GridManager::GenerateFileName(sep::GridGeometry eGeometry) const {
  std::string file_name("config/");
  switch (eGeometry) {
    case sep::DIMAN_GRID_GEOMETRY:
      file_name += std::string("diman");
      break;
    case sep::PROHOR_GRID_GEOMTRY:
      file_name += std::string("prohor");
      break;
    default:
      throw("SaveConfiguration: wrong config");
      break;
  }
  file_name += std::string(".grid");
  return file_name;
}

void GridManager::Build(Config* pConfig) {
  switch (pConfig->GetGridGeometryType()) {
    case sep::DIMAN_GRID_GEOMETRY:
      BuildCombTypeGrid(pConfig);
      break;
    case sep::PROHOR_GRID_GEOMTRY:
      BuildHTypeGrid(pConfig);
      break;
    case sep::DEBUG1_GRID_GEOMETRY:
      BuildDebugTypeGrid(pConfig);
      break;
    default:
      break;
  }
}

void GridManager::BuildDebugTypeGrid(Config* pConfig) {
  std::cout << "Building debug type grid" << std::endl;
  InitEmptyBox(pConfig->GetGridSize());
  AddBox(Vector3i(), pConfig->GetGridSize(), Vector3b(false, false, false), true, 1.0, true);
  //  SetBox(Vector3i(3, 3, 0), Vector3i(4, 4, 1), sep::EMPTY_CELL, 0.4);
  AddBox(Vector3i(3, 3, 0), Vector3i(4, 4, 1), Vector3b(false, false, false), true, 0.4, false);
}

void GridManager::BuildCombTypeGrid(Config* pConfig) {
  std::cout << "Building comb type grid" << std::endl;
  InitEmptyBox(pConfig->GetGridSize());
  AddBox(Vector3i(), pConfig->GetGridSize(), Vector3b(false, false, false), true, 0.5, true);
  
  const Vector3i& vGSize = pConfig->GetGridSize();
  
  //// Test 1: Done
  //// Left and Right walls are 1.0
  //for (int y = 0; y < vGSize.y(); y++) {
  //  m_vCells[0][y][0]->m_cInitCond.Temperature = 1.0;
  //  m_vCells[vGSize.x()-1][y][0]->m_cInitCond.Temperature = 1.0;
  //}

  // Blocks
  // +1 - Hack for small grid (stupid, sorry)
  Vector3i vBlockSize = Vector3i(vGSize.x()*4/8, vGSize.y()*2/8, vGSize.z());
  Vector3i vBlockStart1 = Vector3i(vGSize.x()*3/8-1, vGSize.y()*1/8+1, 0);
  Vector3i vBlockStart2 = Vector3i(vGSize.x()*1/8+1, vGSize.y()*5/8-1, 0);
  AddBox(vBlockStart1, vBlockSize, Vector3b(false, false, false), true, 1.0, false);
  AddBox(vBlockStart2, vBlockSize, Vector3b(false, false, false), true, 0.5, false);

  // Right up and down corners
  m_vCells[vGSize.x() - 1][0][0]->m_eType = sep::FAKE_CELL;
  m_vCells[vGSize.x() - 1][vGSize.y() - 1][0]->m_eType = sep::FAKE_CELL;
  
  // Left up and down corners. Only without vessel!
//  if (!pConfig->GetUseVessels()) {
    m_vCells[0][0][0]->m_eType = sep::FAKE_CELL;
    m_vCells[0][vGSize.y() - 1][0]->m_eType = sep::FAKE_CELL;
//  }
  
  // Should be good for vessel
  if (pConfig->GetUseVessels()) {
    SetVesselBorderBox(Vector3i(0, 1, 0), Vector3i(1, vGSize.y() - 2, 1), true, 0, 1.0);
  }
  
  // Should be good looping with vessel
  if (pConfig->GetUseLooping() && pConfig->GetUseVessels()) {
    SetLoopedBox(Vector3i(), Vector3i(vGSize.x() - 1, 1, 1), true, 0.5);
    SetLoopedBox(Vector3i(0, vGSize.y() - 1, 0), Vector3i(vGSize.x() - 1, 1, 1), false, 0.5);
  }

  // Looping without vessel
  if (pConfig->GetUseLooping() && !pConfig->GetUseVessels()) {
    SetLoopedBox(Vector3i(1, 0, 0), Vector3i(vGSize.x() - 2, 1, 1), true, 0.5);
    SetLoopedBox(Vector3i(1, vGSize.y() - 1, 0), Vector3i(vGSize.x() - 2, 1, 1), false, 0.5);
  }
  

}

void GridManager::BuildHTypeGrid(Config* pConfig) {
  std::cout << "Building H type grid" << std::endl;
  InitEmptyBox(pConfig->GetGridSize());
  bool flat_z = true;
  AddBox(Vector3i(), pConfig->GetGridSize(), Vector3b(false, false, false), flat_z, 0.5, false);
  
  const Vector3i& vGSize = pConfig->GetGridSize();
  
  int n, m, p, D, l, d, h;
  int gaps_q;
  
  n = vGSize.x();
  m = vGSize.y();
  p = vGSize.z();
  
  D = m / 3;
  l = m - 2*D;
  d = 4;
  h = 3;
  gaps_q = 3;

  
  // Add 2D case
  if (flat_z) {
    p = 1;
  }
  
  int x_space = (n - ((gaps_q - 1)*(d + h) + d))/2;
  

  Vector3i start;
  Vector3i size;
  Vector3b without_fakes;
  
  // Add first box
  
  without_fakes[sep::X] = false;
  without_fakes[sep::Y] = false;
  without_fakes[sep::Z] = false;
  
  start[sep::X] = 0;
  start[sep::Y] = D + l;
  start[sep::Z] = 0;
  
  size[sep::X] = n;
  size[sep::Y] = D;
  size[sep::Z] = D;
  
  AddBox(start, size, without_fakes, flat_z, 0.8, true);
  
  // Add second box
  
  without_fakes[sep::X] = false;
  without_fakes[sep::Y] = false;
  without_fakes[sep::Z] = false;
  
  start[sep::X] = 0;
  start[sep::Y] = 0;
  start[sep::Z] = 0;
  
  size[sep::X] = n;
  size[sep::Y] = D;
  size[sep::Z] = D;
  
  AddBox(start, size, without_fakes, flat_z, 1.0, true);
  
  // Add gaps boxes
  
  without_fakes[sep::X] = false;
  without_fakes[sep::Y] = true;
  without_fakes[sep::Z] = false;
  
  for (int i=0; i<gaps_q; i++) {
    
    start[sep::X] = x_space + (d + h) * i;
    start[sep::Y] = D - 2;  // -2 because of fake cells
    start[sep::Z] = D/2 - d/2;
    
    size[sep::X] = d + 2; // +2 because of fake cells
    size[sep::Y] = l + 4; // +4 because of fake cells
    size[sep::Z] = d + 2; // +2 because of fake cells
    
    AddBox(start, size, without_fakes, flat_z, 0.9, true);
  }  
}

void GridManager::InitEmptyBox(const Vector3i& vSize) {
  for (int x = 0; x < vSize.x(); x++) {
    std::vector<std::vector<std::shared_ptr<InitCellData>> > vec_yz;
    m_vCells.push_back(vec_yz);
    
    for (int y = 0; y < vSize.y(); y++) {
      std::vector<std::shared_ptr<InitCellData>> vec_z;
      m_vCells[x].push_back(vec_z);
      
      for (int z = 0; z < vSize.z(); z++) {
        m_vCells[x][y].push_back(std::shared_ptr<InitCellData>(new InitCellData(sep::EMPTY_CELL)));
      }
    }
  }
}

void GridManager::SetBox(const Vector3i& vStart, const Vector3i& vSize, sep::CellType eType, double dWallT) {
  for (int x = vStart.x(); x < vStart.x() + vSize.x(); x++) {
    for (int y = vStart.y(); y < vStart.y() + vSize.y(); y++) {
      for (int z = vStart.z(); z < vStart.z() + vSize.z(); z++) {
        m_vCells[x][y][z]->m_eType = eType;
        m_vCells[x][y][z]->m_cInitCond.Temperature = dWallT;
      }
    }
  }
}

void GridManager::SetLoopedBox(const Vector3i& vStart, const Vector3i& vSize, bool bIsLoopedDown, double dT) {
  for (int x = vStart.x(); x < vStart.x() + vSize.x(); x++) {
    for (int y = vStart.y(); y < vStart.y() + vSize.y(); y++) {
      for (int z = vStart.z(); z < vStart.z() + vSize.z(); z++) {
        m_vCells[x][y][z]->m_bIsLoopedCell = true;
        m_vCells[x][y][z]->m_eType = sep::NORMAL_CELL;
        m_vCells[x][y][z]->m_bIsLoopedDown = bIsLoopedDown;
        m_vCells[x][y][z]->m_cInitCond.Temperature = dT;
      }
    }
  }
}

void GridManager::SetVesselBorderBox(const Vector3i& vStart, const Vector3i& vSize, bool bIsVesselLeft, int iVesselNumber, double dT) {
  for (int x = vStart.x(); x < vStart.x() + vSize.x(); x++) {
    for (int y = vStart.y(); y < vStart.y() + vSize.y(); y++) {
      for (int z = vStart.z(); z < vStart.z() + vSize.z(); z++) {
        m_vCells[x][y][z]->m_bIsVesselCell = true;
        m_vCells[x][y][z]->m_eType = sep::NORMAL_CELL;
        m_vCells[x][y][z]->m_bIsVesselLeft = bIsVesselLeft;
        m_vCells[x][y][z]->m_iVesselNumber = iVesselNumber;
        m_vCells[x][y][z]->m_cInitCond.Temperature = dT;
      }
    }
  }
}

void GridManager::FillInGrid(Config* pConfig) {
  const Vector3i& vSize = pConfig->GetGridSize();
  // Init grid size
  m_pGrid->m_vSize = vSize;
  // For a while
  m_pGrid->m_vWholeSize = vSize;
  m_pGrid->m_vStart = Vector3i();
  for (int x = 0; x < vSize.x(); x++) {
    for (int y = 0; y < vSize.y(); y++) {
      for (int z = 0; z < vSize.z(); z++) {
        if (m_vCells[x][y][z]->m_eType == sep::EMPTY_CELL)
          continue;
        Cell* pCell = new Cell(this);
        m_pGrid->AddCell(std::shared_ptr<Cell>(pCell));
        m_vCells[x][y][z]->m_pCell = pCell;
//        std::cout << "Adding cell("<< x << "," << y << "," << z << ")" << std::endl;
      }
    }
  }
}

Cell* GridManager::GetNeighb(Vector3i vCoord, sep::Axis eAxis, int iSlash) {
  std::vector<int> vSlash;
  vSlash.resize(3, 0);
  vSlash[eAxis] = iSlash;
  const Vector3i& vSize = m_pGrid->GetSize();
  Vector3i vNeibh(vCoord.x() + vSlash[sep::X], vCoord.y() + vSlash[sep::Y], vCoord.z() + vSlash[sep::Z]);
  if (vNeibh.x() >= vSize.x() ||
      vNeibh.y() >= vSize.y() ||
      vNeibh.z() >= vSize.z() ||
      vNeibh.x() < 0 || vNeibh.y() < 0 || vNeibh.z() < 0 ||
      m_vCells[vNeibh.x()][vNeibh.y()][vNeibh.z()]->m_eType == sep::EMPTY_CELL ||
      (m_vCells[vCoord.x()][vCoord.y()][vCoord.z()]->m_eType == sep::FAKE_CELL && m_vCells[vNeibh.x()][vNeibh.y()][vNeibh.z()]->m_eType == sep::FAKE_CELL))
    return NULL;
  return m_vCells[vNeibh.x()][vNeibh.y()][vNeibh.z()]->m_pCell;
}

void GridManager::LinkCells(Config* pConfig) {
  const Vector3i& vSize = pConfig->GetGridSize();
  for (int x = 0; x < vSize.x(); x++) {
    for (int y = 0; y < vSize.y(); y++) {
      for (int z = 0; z < vSize.z(); z++) {
        Cell* cell = m_vCells[x][y][z]->m_pCell;
        InitCellData* init_cell = m_vCells[x][y][z].get();
        const MacroData& init_cond = m_vCells[x][y][z]->m_cInitCond;
        if (init_cell->m_eType == sep::EMPTY_CELL)
          continue;
        // Init vectors
        cell->m_vPrev[sep::X].push_back(NULL);
        cell->m_vPrev[sep::Y].push_back(NULL);
        cell->m_vPrev[sep::Z].push_back(NULL);
        cell->m_vNext = cell->m_vPrev;
        
        // Link
        Vector3i vCoord(x, y, z);
        for (int ax = 0; ax < 3; ax++) {
          cell->m_vPrev[ax][0] = GetNeighb(vCoord, (sep::Axis)ax, -1);
          cell->m_vNext[ax][0] = GetNeighb(vCoord, (sep::Axis)ax, +1);
        }
        
        if (init_cell->m_bIsLoopedCell) {
          if (init_cell->m_bIsLoopedDown)
            cell->m_vPrev[sep::Y][0] = m_vCells[x][vSize.y() - 1][z]->m_pCell;
          else
            cell->m_vNext[sep::Y][0] = m_vCells[x][0][z]->m_pCell;
        }
        
        if (init_cell->m_bIsVesselCell) {
          VesselGrid* pVess = nullptr;
          int iVessN = init_cell->m_iVesselNumber;
          if (init_cell->m_bIsVesselLeft) {
            pVess = m_vLeftVess[iVessN].get();
            // TODO: Use vessel coord, not just y
            int iVessStartY = pVess->getVesselGridInfo()->vStart.y();
            Cell* pVessCell = pVess->GetBorderCell(y - iVessStartY);
            cell->m_vPrev[sep::X][0] = pVessCell;
            pVessCell->m_vNext[sep::X].push_back(cell);
          }
          else {
            pVess = m_vRightVess[iVessN].get();
            Cell* pVessCell = pVess->GetBorderCell(y);
            cell->m_vNext[sep::X][0] = pVessCell;
            pVessCell->m_vPrev[sep::X].push_back(cell);
          }
        }
        
        // Remove nulls
        for (int ax = 0; ax < 3; ax++) {
          if (!cell->m_vPrev[ax][0])
            cell->m_vPrev[ax].clear();
          if (!cell->m_vNext[ax][0])
            cell->m_vNext[ax].clear();
        }
        
        // Set parameters
        Vector3d vAreaStep(0.1, 0.1, 0.1);
        cell->setParameters(init_cond.Concentration, init_cond.Temperature, vAreaStep);
        
        cell->Init();
      }
    }
  }
}

bool GridManager::IsConer(Vector3i vStart, Vector3i vEnd, Vector3i vP) {
  // Only 2D!
  Vector3i vCorn0 = Vector3i(vStart.x(), vEnd.y(), vStart.z());
  Vector3i vCorn1 = Vector3i(vEnd.x(), vStart.y(), vStart.z());
  if (vP == vStart || vP == vEnd || vP == vCorn0 || vP == vCorn1)
    return true;
  return false;
}

// Add box with fake cells around it
void GridManager::AddBox(Vector3i vStart, Vector3i vSize, Vector3b vWithoutFakes, bool bFlatZ, double dWallT, bool bGasBox) {
  
  int n, m, p;
  
  // add 2D case
  if (bFlatZ) {
    vStart.z() = 0;
    vSize.z() = 1;
    vWithoutFakes.z() = true;
  }
  
  n = vStart.x() + vSize.x();
  m = vStart.y() + vSize.y();
  p = vStart.z() + vSize.z();
  
  for (int i = vStart.x(); i < n; i++) {
    
    for (int j = vStart.y(); j < m; j++) {
      
      for (int k = vStart.z(); k < p; k++) {
        
        InitCellData* cell = m_vCells[i][j][k].get();
        
        // one rule: (do we need this?)
        // if cell is already exist and it's type is normal
        // we do not overwrite it
        if (bGasBox && cell->m_eType == sep::NORMAL_CELL)
          continue;
        
        if (i == vStart.x() || i == n-1 ||
            j == vStart.y() || j == m-1 ||
            ((k == vStart.z() || k == p-1) &&
             !bFlatZ /* not to skip the first row */)) {
              // fake cells
              
              // it's not the repeat of condition, it's logically right
              if (((i == vStart.x() || i == n-1) && !vWithoutFakes.x()) ||
                  ((j == vStart.y() || j == m-1) && !vWithoutFakes.y()) ||
                  ((k == vStart.z() || k == p-1) && !vWithoutFakes.z())) {
                // only for some edges
                
                cell->m_eType = sep::FAKE_CELL;
              }
            }
        else {
          // normal cells
          if (bGasBox)
            cell->m_eType = sep::NORMAL_CELL;
          else
            cell->m_eType = sep::EMPTY_CELL;
        }
        
        if (bGasBox && IsConer(vStart, vStart + vSize + Vector3i(-1, -1, -1), Vector3i(i, j, k)))
          cell->m_eType = sep::EMPTY_CELL;
        
        m_vCells[i][j][k]->m_cInitCond.Temperature = dWallT;
      }
    }
  }
}

void GridManager::Print(sep::Axis axis) {
  Config* pConfig = GetConfig();
  const Vector3i& vGridSize = pConfig->GetGridSize();
  const Vector3i& vStartOutGrid = pConfig->GetOutputGridStart();
  const Vector3i& vOutputSize = pConfig->GetOutputSize();
  
  for (int y = 0; y < vOutputSize.y(); y++) {
    for (int x = 0; x < vOutputSize.x(); x++) {
      int z = 0;
      
      int g_x = x - vStartOutGrid.x();
      int g_y = y - vStartOutGrid.y();
      int g_z = z - vStartOutGrid.z();
      // Edge cells are faked
      int iEdge = 0;
      // Out grid
      if (g_x >= 0 + iEdge && g_y >= 0 + iEdge && g_z >= 0 + iEdge &&
          g_x < vGridSize.x() - iEdge && g_y < vGridSize.y() - iEdge && g_z < vGridSize.z() - iEdge) {

        Cell* cell = m_vCells[g_x][g_y][g_z]->m_pCell;
        if (!cell) {
          std::cout << "x" << " ";
          continue;
        }
        std::cout << cell->m_vType[axis] << " ";
      } else {
        // Looking for vessels
        
        if (!pConfig->GetUseVessels()) {
          std::cout << "x" << " ";
          continue;
        }
        
        // For all vessels
        for (int iLeftVess = 0; iLeftVess < 2; iLeftVess++) {
          const std::vector<std::shared_ptr<VesselGrid>>& vVessels = GetLeftRightVessels(iLeftVess);
        
          for (auto& pVessel : vVessels) {
            
            const std::vector< std::vector<Cell*> >& vVessCells = pVessel->GetPrintVector();
            const Vector2i& vVesselSize2D = pVessel->GetPrintVectorSize();
            Vector3i vVesselSize(vVesselSize2D.x(), vVesselSize2D.y(), 1);
            Vector3i vVesselStart = pVessel->getVesselGridInfo()->vStart;
            vVesselStart.x() = vStartOutGrid.x() - vVesselSize.x();
        
            int v_x = x - vVesselStart.x();
            int v_y = y - vVesselStart.y();
            int v_z = z - vVesselStart.z();
            
            iEdge = 0;
            if (v_x >= 0 + iEdge && v_y >= 0 + iEdge && v_z >= 0 + iEdge &&
                v_x < vVesselSize.x() - iEdge && v_y < vVesselSize.y() - iEdge && v_z < vVesselSize.z() - iEdge) {
              
              Cell* cell = vVessCells[v_x][v_y];
              if (!cell) {
                std::cout << "x" << " ";
                continue;
              }
              std::cout << cell->m_vType[axis] << " ";
            } else {
              std::cout << "x" << " ";
            }
          }
        }
      }
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

void GridManager::InitVessels() {
  m_vLeftVess.push_back(std::shared_ptr<VesselGrid>(new LeftVesselGrid));
  VesselGrid* lvg = m_vLeftVess[0].get();
  Config* pConfig = GetConfig();
  // Need Nx
  int iNx = pConfig->GetGridSize().y();
  int iNy = pConfig->GetGridSize().y();
  int iNz = 1;
  lvg->setGridManager(this);
  lvg->getVesselGridInfo()->dStartConcentration = 1.0;
  lvg->getVesselGridInfo()->dStartTemperature = 1.0;
  lvg->getVesselGridInfo()->iAdditionalLenght = 0;
  lvg->getVesselGridInfo()->iNy = iNy;
  lvg->getVesselGridInfo()->iNz = iNz;
  lvg->getVesselGridInfo()->vAreastep = Vector3d(0.1, 0.1, 0.1);
  lvg->getVesselGridInfo()->vStart = Vector3i();
  lvg->getVesselGridInfo()->vSize = Vector3i(iNx, iNy, iNz);
  
  lvg->SetVesselGridType(VesselGrid::VGT_NORMAL);
  lvg->CreateAndLinkVessel();
}
















