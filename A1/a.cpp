#include <bits/stdc++.h>
using namespace std;
int isPossible(vector<int> & books, int threshold, int c){
    queue<int> covered;
    if (c % 2 == 0) c++;
    int total = 0;
    for (int i = 0; i < books.size(); i++){
        if (books[i] >= threshold){
            covered.push(1);
            total++;
        }else{
            covered.push(0);
        }
        if (covered.size() == c){
            if (total > c / 2) return 1;
            total -= covered.front();
            covered.pop();
        }
    }
    return 0;
}
int main(){
    int n, c;
    cin >> n ;
    vector<int> books(n);
    vector<int> cpy(n);
    for (int i = 0; i < n; i++){
        cin >> books[i];
        cpy[i] = books[i];
    }
    cin >> c;
    sort(cpy.begin(), cpy.end());
    int l = 0, r = n-1, mid;
    while (l != r){
        mid = (l + r + 1) / 2;
        if (isPossible(books, cpy[mid], c)){
            // cout << cpy[mid] << endl;
            l = mid;
        }else{
            r = mid - 1;
        }
    }
    cout << (n - l) << endl;
}