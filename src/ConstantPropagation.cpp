// Test case - Copy propagation
int constantPropagation() {
    int b = 4, c, d, e;
    c = d; // copy propagation
    e = c + b;
    return e;
}
