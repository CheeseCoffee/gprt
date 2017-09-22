#include <chrono>

#include "Solver.h"
#include "grid/GridMaker.h"
#include "grid/Cell.h"
#include "grid/CellData.h"
#include "utilities/Parallel.h"
#include "parameters/Gas.h"
#include "parameters/BetaChain.h"
#include "integral/ci.hpp"
#include "ResultsWriter.h"

Solver::Solver() {
    _config = Config::getInstance();
    _impulse = _config->getImpulse();
    _writer = new ResultsWriter();
    _maker = new GridMaker();
}

void Solver::init() {

    // create grid
    Grid<CellData>* dataGrid = _maker->makeGrid(Vector2u(
            _config->getGridSize().x(),
            _config->getGridSize().y()
    ));

    // init cell grid
    _grid = new Grid<Cell>(dataGrid->getSize());
    for (unsigned int i = 0; i < dataGrid->getCount(); i++) {
        CellData* data = dataGrid->getByIndex(i);
        if (data != nullptr) {
            auto* cell = new Cell(data);
            cell->init();
            _grid->setByIndex(i, cell);
        }
    }

    // link
    for (unsigned int x = 0; x < _grid->getSize().x(); x++) {
        for (unsigned int y = 0; y < _grid->getSize().y(); y++) {
            Cell* cell = _grid->get(x, y);
            if (cell != nullptr) {
                std::vector<Cell*> linkCells{4};

                if (x != 0) {
                    linkCells[0] = _grid->get(x - 1, y);
                }
                if (x != _grid->getSize().x() - 1) {
                    linkCells[1] = _grid->get(x + 1, y);
                }
                if (y != 0) {
                    linkCells[2] = _grid->get(x, y - 1);
                }
                if (y != _grid->getSize().y() - 1) {
                    linkCells[3] = _grid->get(x, y + 1);
                }

                if (cell->getData()->isFake() == true) {
                    for (auto& linkCell : linkCells) {
                        if (linkCell != nullptr && linkCell->getData()->isFake() == true) {
                            linkCell = nullptr;
                        }
                    }
                }

                cell->link(sep::Axis::X, linkCells[0], linkCells[1]);
                cell->link(sep::Axis::Y, linkCells[2], linkCells[3]);
            }
        }
    }
}

void Solver::run() {

    // Compute cell type for each axis
    initType();

//    std::cout << *_grid << std::endl;

    if (_config->isUseIntegral()) {
        ci::Potential* potential = new ci::HSPotential;
        ci::init(potential, ci::NO_SYMM);
    }

//    auto startTime = std::chrono::steady_clock::now();

    if (Parallel::isMaster() == true) {
        std::cout << std::endl;
    }
    unsigned int maxIteration = _config->getMaxIteration();
    for (unsigned int iteration = 0; iteration < maxIteration; iteration++) {

        // transfer
        makeTransfer();

        // integral
        if (_config->isUseIntegral()) {
            int iGasesNum = _config->getGasesCount();
            double dTimestep = _config->getTimestep();

            if (iGasesNum == 1) {
                makeIntegral(0, 0, dTimestep);
            } else if (iGasesNum == 2) {
                makeIntegral(0, 0, dTimestep);
                makeIntegral(0, 1, dTimestep);
            } else if (iGasesNum >= 3) {
                makeIntegral(0, 0, dTimestep);
                makeIntegral(0, 1, dTimestep);
                makeIntegral(0, 2, dTimestep);
            }
        }

        // beta decay
        if (_config->isUseBetaChains()) {
            for (int i = 0; i < _config->getBetaChainsCount(); i++) {
                auto& item = _config->getBetaChains()[i];
                makeBetaDecay(item.iGasIndex1, item.iGasIndex2, item.dLambda1);
                makeBetaDecay(item.iGasIndex2, item.iGasIndex3, item.dLambda2);
            }
        }

        checkCells();

        if (Parallel::isUsingMPI() == true && Parallel::getSize() > 1) {
            _maker->syncGrid(_grid);
        }

//        writeResults(iteration);

        //    auto now = std::chrono::steady_clock::now();
        //    auto wholeTime = std::chrono::duration_cast<std::chrono::seconds>(now - _startTime).count();
        //    auto iterationTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - _startTime).count();
        //
        //    std::cout << "run() : " << iteration << "/" << maxIteration << std::endl;
        //    std::cout << "Iteration time: " << iterationTime << " ms" << std::endl;
        //    std::cout << "Inner time: " << iteration * _config->getNormalizer()->restore(static_cast<const double&>(_config->getTimestep()), Normalizer::Type::TIME) << " s" << std::endl;
        //    std::cout << "Real time: " << wholeTime << " s" << std::endl;
        //    std::cout << "Remaining time: ";
        //    if (iteration == 0) {
        //        std::cout << "Unknown";
        //    } else {
        //        std::cout << wholeTime * (_config->getMaxIteration() - iteration) / iteration / 60 << " m";
        //    }
        //    std::cout << std::endl;
        //    std::cout << std::endl;

        if (Parallel::isMaster() == true) {
            auto percent = (unsigned int) (1.0 * iteration / maxIteration * 100);

            std::cout << '\r';
            std::cout << "[" << std::string(percent, '#') << std::string(100 - percent, '-') << "] ";
            std::cout << percent << "%";
            std::cout.flush();
        }
    }
    if (Parallel::isMaster() == true) {
        std::cout << std::endl << "Done" << std::endl;
    }
}

void Solver::initType() {
    const std::vector<Cell*>& cells = _grid->getValues();

    // compute type
    for (auto item : cells) {
        if (item != nullptr && item->getData()->isFakeParallel() == false) {
            item->computeType(sep::X);
            item->computeType(sep::Y);
        }
    }
}

void Solver::makeTransfer() {
    const std::vector<Cell*>& cells = _grid->getValues();

    // Transfer X
    for (auto item : cells) if (item != nullptr && item->getData()->isFakeParallel() == false) item->computeHalf(sep::X);
    for (auto item : cells) if (item != nullptr && item->getData()->isFakeParallel() == false) item->computeValue(sep::X);

    // Transfer Y
    for (auto item : cells) if (item != nullptr && item->getData()->isFakeParallel() == false) item->computeHalf(sep::Y);
    for (auto item : cells) if (item != nullptr && item->getData()->isFakeParallel() == false) item->computeValue(sep::Y);
}

void Solver::makeIntegral(unsigned int gi0, unsigned int gi1, double timestep) {
    const std::vector<Gas>& vGases = _config->getGases();

    ci::Particle cParticle{};
    cParticle.d = 1.;

    ci::gen(timestep, 50000,
            _impulse->getResolution() / 2, _impulse->getResolution() / 2,
            _impulse->getXYZ2I(), _impulse->getXYZ2I(),
            _impulse->getMaxImpulse() / (_impulse->getResolution() / 2),
            vGases[gi0].getMass(), vGases[gi1].getMass(),
            cParticle, cParticle);

    const std::vector<Cell*>& cells = _grid->getValues();
    for (auto item : cells) {
        if (item != nullptr && item->getData()->isFakeParallel() == false) {
            item->computeIntegral(gi0, gi1);
        }
    }
}

void Solver::makeBetaDecay(unsigned int gi0, unsigned int gi1, double lambda) {
    const std::vector<Cell*>& cells = _grid->getValues();
    for (auto item : cells) {
        if (item != nullptr && item->getData()->isFakeParallel() == false) {
            item->computeBetaDecay(gi0, gi1, lambda);
        }
    }
}

void Solver::checkCells() {
    const std::vector<Cell*>& cells = _grid->getValues();
    for (auto item : cells) {
        if (item != nullptr && item->getData()->isFakeParallel() == false) {
            item->checkInnerValuesRange();
        }
    }
}

void Solver::writeResults(unsigned int iteration) {
    Grid<CellParameters>* resultParams = new Grid<CellParameters>(_grid->getSize());
    for (unsigned int i = 0; i < _grid->getCount(); i++) {
        auto* cell = _grid->getByIndex(i);
        if (cell != nullptr) {
            auto& params = cell->getResultParams();
            resultParams->setByIndex(i, &params);
        }
    }
    _writer->writeAll(resultParams, iteration);
}