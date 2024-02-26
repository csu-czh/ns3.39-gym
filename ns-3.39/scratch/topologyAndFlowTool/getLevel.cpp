#include <iostream>
#include <vector>
#include <map>
using namespace std;
int l,a,b;
vector<pair<int,int>>ve[6];
map<int,int>boss;
int main(){
    while(cin>>l>>a>>b){
        ve[l].push_back(make_pair(a,b));
    }

    for(int i=1;i<5;i++){
        for(auto it = ve[i].begin(); it!= ve[i].end();it++){
            int a  = it->first;
            int b = it->second;
            if(boss.find(a) == boss.end()){
                boss[a] = i;
            }
            boss[b] = boss[a];
        }
    }
    int num[6] = {0};
    int pum[6] = {0};
    for(int i=1;i<=5;i++){
        for(auto it = ve[i].begin(); it!= ve[i].end();it++){
            num[boss[it->first]]++;
            if(boss[it->first] == i){
                pum[i]++;
            }
        }
    }
    for(int i=1;i<=5;i++){
        cout<<num[i]<<" "<<pum[i]<<endl;
    }
    cout<<endl;
    return 0;
}