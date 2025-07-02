#pragma once

#include "veins/veins.h"
#include <cmath>
#include <string>

inline std::string calcularDireccion(const veins::Coord& anterior, const veins::Coord& actual) {

    if (anterior == actual)
        return "Desconocida";  // No se ha movido o es la primera vez que se lo ve

    veins::Coord delta = anterior - actual;
    double angulo = atan2(delta.y, delta.x) * 180 / M_PI;
    if (angulo < 0) angulo += 360;

    if (angulo >= 45 && angulo < 135)
        return "Norte";
    else if (angulo >= 135 && angulo < 225)
        return "Oeste";
    else if (angulo >= 225 && angulo < 315)
        return "Sur";
    else
        return "Este";
}

