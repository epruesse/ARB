#!/bin/bash
#
# extracts those parts of eigen-2.0.16 needed by RNACMA
# (in order to reduce size 3.8M -> 1.5M)

SOURCE=./eigen-eigen-2.0.16
DEST=./eigen

eigencp() {
    cp -pr $SOURCE/$1 $DEST/$1 || exit
}

mkdir -p $DEST/Eigen
mkdir -p $DEST/Eigen/src
mkdir -p $DEST/Eigen/src/Array
mkdir -p $DEST/Eigen/src/Cholesky
mkdir -p $DEST/Eigen/src/Core
mkdir -p $DEST/Eigen/src/Core/util
mkdir -p $DEST/Eigen/src/Geometry
mkdir -p $DEST/Eigen/src/LU
mkdir -p $DEST/Eigen/src/LeastSquares
mkdir -p $DEST/Eigen/src/QR
mkdir -p $DEST/Eigen/src/SVD
mkdir -p $DEST/Eigen/src/Sparse

eigencp Eigen/src/Core/arch

eigencp COPYING
eigencp COPYING.LESSER
eigencp Eigen/Array
eigencp Eigen/Cholesky
eigencp Eigen/Core
eigencp Eigen/Dense
eigencp Eigen/Eigen
eigencp Eigen/Geometry
eigencp Eigen/LU
eigencp Eigen/LeastSquares
eigencp Eigen/QR
eigencp Eigen/SVD
eigencp Eigen/Sparse
eigencp Eigen/src/Array/BooleanRedux.h
eigencp Eigen/src/Array/CwiseOperators.h
eigencp Eigen/src/Array/Functors.h
eigencp Eigen/src/Array/Norms.h
eigencp Eigen/src/Array/PartialRedux.h
eigencp Eigen/src/Array/Random.h
eigencp Eigen/src/Array/Select.h
eigencp Eigen/src/Cholesky/LDLT.h
eigencp Eigen/src/Cholesky/LLT.h
eigencp Eigen/src/Core/Assign.h
eigencp Eigen/src/Core/Block.h
eigencp Eigen/src/Core/CacheFriendlyProduct.h
eigencp Eigen/src/Core/Coeffs.h
eigencp Eigen/src/Core/CommaInitializer.h
eigencp Eigen/src/Core/Cwise.h
eigencp Eigen/src/Core/CwiseBinaryOp.h
eigencp Eigen/src/Core/CwiseNullaryOp.h
eigencp Eigen/src/Core/CwiseUnaryOp.h
eigencp Eigen/src/Core/DiagonalCoeffs.h
eigencp Eigen/src/Core/DiagonalMatrix.h
eigencp Eigen/src/Core/DiagonalProduct.h
eigencp Eigen/src/Core/Dot.h
eigencp Eigen/src/Core/Flagged.h
eigencp Eigen/src/Core/Functors.h
eigencp Eigen/src/Core/Fuzzy.h
eigencp Eigen/src/Core/GenericPacketMath.h
eigencp Eigen/src/Core/IO.h
eigencp Eigen/src/Core/Map.h
eigencp Eigen/src/Core/MapBase.h
eigencp Eigen/src/Core/MathFunctions.h
eigencp Eigen/src/Core/Matrix.h
eigencp Eigen/src/Core/MatrixBase.h
eigencp Eigen/src/Core/MatrixStorage.h
eigencp Eigen/src/Core/Minor.h
eigencp Eigen/src/Core/NestByValue.h
eigencp Eigen/src/Core/NumTraits.h
eigencp Eigen/src/Core/Part.h
eigencp Eigen/src/Core/Product.h
eigencp Eigen/src/Core/Redux.h
eigencp Eigen/src/Core/SolveTriangular.h
eigencp Eigen/src/Core/Sum.h
eigencp Eigen/src/Core/Swap.h
eigencp Eigen/src/Core/Transpose.h
eigencp Eigen/src/Core/Visitor.h
eigencp Eigen/src/Core/util/Constants.h
eigencp Eigen/src/Core/util/DisableMSVCWarnings.h
eigencp Eigen/src/Core/util/EnableMSVCWarnings.h
eigencp Eigen/src/Core/util/ForwardDeclarations.h
eigencp Eigen/src/Core/util/Macros.h
eigencp Eigen/src/Core/util/Memory.h
eigencp Eigen/src/Core/util/Meta.h
eigencp Eigen/src/Core/util/StaticAssert.h
eigencp Eigen/src/Core/util/XprHelper.h
eigencp Eigen/src/Geometry/AlignedBox.h
eigencp Eigen/src/Geometry/AngleAxis.h
eigencp Eigen/src/Geometry/EulerAngles.h
eigencp Eigen/src/Geometry/Hyperplane.h
eigencp Eigen/src/Geometry/OrthoMethods.h
eigencp Eigen/src/Geometry/ParametrizedLine.h
eigencp Eigen/src/Geometry/Quaternion.h
eigencp Eigen/src/Geometry/Rotation2D.h
eigencp Eigen/src/Geometry/RotationBase.h
eigencp Eigen/src/Geometry/Scaling.h
eigencp Eigen/src/Geometry/Transform.h
eigencp Eigen/src/Geometry/Translation.h
eigencp Eigen/src/LU/Determinant.h
eigencp Eigen/src/LU/Inverse.h
eigencp Eigen/src/LU/LU.h
eigencp Eigen/src/LeastSquares/LeastSquares.h
eigencp Eigen/src/QR/EigenSolver.h
eigencp Eigen/src/QR/HessenbergDecomposition.h
eigencp Eigen/src/QR/QR.h
eigencp Eigen/src/QR/SelfAdjointEigenSolver.h
eigencp Eigen/src/QR/Tridiagonalization.h
eigencp Eigen/src/SVD/SVD.h
eigencp Eigen/src/Sparse/AmbiVector.h
eigencp Eigen/src/Sparse/CompressedStorage.h
eigencp Eigen/src/Sparse/CoreIterators.h
eigencp Eigen/src/Sparse/DynamicSparseMatrix.h
eigencp Eigen/src/Sparse/MappedSparseMatrix.h
eigencp Eigen/src/Sparse/RandomSetter.h
eigencp Eigen/src/Sparse/SparseAssign.h
eigencp Eigen/src/Sparse/SparseBlock.h
eigencp Eigen/src/Sparse/SparseCwise.h
eigencp Eigen/src/Sparse/SparseCwiseBinaryOp.h
eigencp Eigen/src/Sparse/SparseCwiseUnaryOp.h
eigencp Eigen/src/Sparse/SparseDiagonalProduct.h
eigencp Eigen/src/Sparse/SparseDot.h
eigencp Eigen/src/Sparse/SparseFlagged.h
eigencp Eigen/src/Sparse/SparseFuzzy.h
eigencp Eigen/src/Sparse/SparseLDLT.h
eigencp Eigen/src/Sparse/SparseLLT.h
eigencp Eigen/src/Sparse/SparseLU.h
eigencp Eigen/src/Sparse/SparseMatrix.h
eigencp Eigen/src/Sparse/SparseMatrixBase.h
eigencp Eigen/src/Sparse/SparseProduct.h
eigencp Eigen/src/Sparse/SparseRedux.h
eigencp Eigen/src/Sparse/SparseTranspose.h
eigencp Eigen/src/Sparse/SparseUtil.h
eigencp Eigen/src/Sparse/SparseVector.h
eigencp Eigen/src/Sparse/TriangularSolver.h

tar -zcvf eigen.tgz eigen/
rm -rf eigen/

