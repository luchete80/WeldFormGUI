#!/usr/bin/env bash

export WELDFORM_SOLVER_EDITION=full

echo "WELDFORM_SOLVER_EDITION=${WELDFORM_SOLVER_EDITION}"
echo "Student limit variables left unchanged:"
echo "  WELDFORM_STD_NODE_LIMIT=${WELDFORM_STD_NODE_LIMIT:-<unset>}"
echo "  WELDFORM_STD_ALLOW_THERMAL=${WELDFORM_STD_ALLOW_THERMAL:-<unset>}"
echo
echo "Run with:"
echo "  source scripts/use_solver_full.sh"
