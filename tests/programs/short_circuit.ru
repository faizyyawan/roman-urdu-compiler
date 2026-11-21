adad zero = 0 kardo

agar (zero && (10 / zero)) {
    dikhao("wrong") kardo
}

agar (1 || (10 / zero)) {
    dikhao("short circuit ok") kardo
}

wapis 0 kardo
