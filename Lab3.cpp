// File: hybrid_3d.cpp
// 3D Hybrid Transformations with Homogeneous Coordinates
// Supports: Translation, Scaling, Rotation (X/Y/Z), Reflection (planes/origin), Shearing (all pairs)
// Compose arbitrary sequences (hybrid) and apply to an object (cube or custom points)

#include <bits/stdc++.h>
using namespace std;

using Mat4 = array<array<double,4>,4>;
using Vec4 = array<double,4>;

static inline double deg2rad(double d){ return d * M_PI / 180.0; }

Mat4 I4(){
    Mat4 m{}; 
    for(int i=0;i<4;i++) m[i][i]=1.0;
    return m;
}

// Matrix multiply: C = A * B (4x4)
Mat4 mul(const Mat4& A, const Mat4& B){
    Mat4 C{};
    for(int i=0;i<4;i++){
        for(int j=0;j<4;j++){
            double s=0;
            for(int k=0;k<4;k++) s += A[i][k]*B[k][j];
            C[i][j]=s;
        }
    }
    return C;
}

// Apply matrix to a point (column vector convention)
Vec4 apply(const Mat4& M, const Vec4& p){
    Vec4 r{};
    for(int i=0;i<4;i++){
        r[i]= M[i][0]*p[0] + M[i][1]*p[1] + M[i][2]*p[2] + M[i][3]*p[3];
    }
    return r;
}

// ----- Basic Transform Constructors -----

Mat4 T(double tx,double ty,double tz){
    Mat4 m = I4();
    m[0][3]=tx; m[1][3]=ty; m[2][3]=tz;
    return m;
}

Mat4 S(double sx,double sy,double sz){
    Mat4 m{};
    m[0][0]=sx; m[1][1]=sy; m[2][2]=sz; m[3][3]=1.0;
    return m;
}

Mat4 Rx(double deg){
    double c=cos(deg2rad(deg)), s=sin(deg2rad(deg));
    Mat4 m = I4();
    m[1][1]=c; m[1][2]=-s;
    m[2][1]=s; m[2][2]= c;
    return m;
}

Mat4 Ry(double deg){
    double c=cos(deg2rad(deg)), s=sin(deg2rad(deg));
    Mat4 m = I4();
    m[0][0]= c; m[0][2]= s;
    m[2][0]=-s; m[2][2]= c;
    return m;
}

Mat4 Rz(double deg){
    double c=cos(deg2rad(deg)), s=sin(deg2rad(deg));
    Mat4 m = I4();
    m[0][0]=c; m[0][1]=-s;
    m[1][0]=s; m[1][1]= c;
    return m;
}

// Reflection about planes/origin (column-vector)
Mat4 ReflectXY(){ auto m=I4(); m[2][2]=-1; return m; } // z -> -z
Mat4 ReflectYZ(){ auto m=I4(); m[0][0]=-1; return m; } // x -> -x
Mat4 ReflectZX(){ auto m=I4(); m[1][1]=-1; return m; } // y -> -y
Mat4 ReflectOrigin(){ auto m=I4(); m[0][0]=m[1][1]=m[2][2]=-1; return m; }

// General shearing:
// x' = x + sh_xy*y + sh_xz*z
// y' = y + sh_yx*x + sh_yz*z
// z' = z + sh_zx*x + sh_zy*y
Mat4 Shear(double sh_xy,double sh_xz,
           double sh_yx,double sh_yz,
           double sh_zx,double sh_zy){
    Mat4 m = I4();
    m[0][1]=sh_xy; m[0][2]=sh_xz;
    m[1][0]=sh_yx; m[1][2]=sh_yz;
    m[2][0]=sh_zx; m[2][1]=sh_zy;
    return m;
}

// ----- Utilities -----

void printMat(const Mat4& M){
    cout.setf(std::ios::fixed); cout<<setprecision(4);
    for(int i=0;i<4;i++){
        for(int j=0;j<4;j++){
            cout<<setw(10)<<M[i][j]<<" ";
        }
        cout<<"\n";
    }
}

void printPts(const vector<Vec4>& P, const string& title){
    cout<<"\n"<<title<<":\n";
    cout.setf(std::ios::fixed); cout<<setprecision(4);
    for(size_t i=0;i<P.size();++i){
        cout<<"["<<setw(2)<<i<<"]  x="<<setw(8)<<P[i][0]
            <<"  y="<<setw(8)<<P[i][1]
            <<"  z="<<setw(8)<<P[i][2]<<"\n";
    }
}

vector<Vec4> makeCube(double s=1.0){
    // cube centered at origin with side length s
    double a = s/2.0;
    vector<Vec4> v;
    for(int dx:{-1,1})
      for(int dy:{-1,1})
        for(int dz:{-1,1})
            v.push_back( Vec4{dx*a, dy*a, dz*a, 1.0} );
    return v; // 8 vertices
}

// Apply matrix to all points (homogeneous divide if w != 1)
vector<Vec4> transformAll(const Mat4& M, const vector<Vec4>& P){
    vector<Vec4> Q; Q.reserve(P.size());
    for(const auto& p: P){
        Vec4 r = apply(M,p);
        if (fabs(r[3])>1e-12){
            r[0]/=r[3]; r[1]/=r[3]; r[2]/=r[3]; r[3]=1.0;
        }
        Q.push_back(r);
    }
    return Q;
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout<<"=== 3D Hybrid Transformations (Homogeneous 4x4) ===\n";
    cout<<"Convention: column vectors, P' = M * P. Composition order matters.\n\n";

    // Choose object
    vector<Vec4> pts;
    cout<<"Choose object:\n";
    cout<<"  1) Unit cube centered at origin\n";
    cout<<"  2) Enter custom points\n";
    cout<<"Enter choice (1/2): ";
    int choice=1; 
    if(!(cin>>choice)) return 0;

    if(choice==2){
        int n; 
        cout<<"How many points? ";
        cin>>n;
        pts.resize(n);
        cout<<"Enter "<<n<<" points as x y z per line:\n";
        for(int i=0;i<n;i++){
            double x,y,z; cin>>x>>y>>z;
            pts[i] = Vec4{x,y,z,1.0};
        }
    } else {
        pts = makeCube(2.0); // side 2 for visibility
    }

    printPts(pts, "Original Points");

    Mat4 M_total = I4();

    while(true){
        cout<<"\nAdd a transform (or 0 to finish):\n";
        cout<<" 1) Translation (tx, ty, tz)\n";
        cout<<" 2) Scaling (sx, sy, sz)\n";
        cout<<" 3) Rotation X (deg)\n";
        cout<<" 4) Rotation Y (deg)\n";
        cout<<" 5) Rotation Z (deg)\n";
        cout<<" 6) Reflection XY (z -> -z)\n";
        cout<<" 7) Reflection YZ (x -> -x)\n";
        cout<<" 8) Reflection ZX (y -> -y)\n";
        cout<<" 9) Reflection Origin (x,y,z -> -x,-y,-z)\n";
        cout<<"10) Shear (sh_xy sh_xz sh_yx sh_yz sh_zx sh_zy)\n";
        cout<<" 0) Apply & print\n";
        cout<<"Choice: ";
        int op; 
        if(!(cin>>op)) return 0;
        if(op==0) break;

        Mat4 M = I4();

        switch(op){
            case 1:{
                double tx,ty,tz; 
                cout<<"tx ty tz: "; cin>>tx>>ty>>tz;
                M = T(tx,ty,tz);
                break;
            }
            case 2:{
                double sx,sy,sz; 
                cout<<"sx sy sz: "; cin>>sx>>sy>>sz;
                M = S(sx,sy,sz);
                break;
            }
            case 3:{
                double a; cout<<"angle (deg): "; cin>>a;
                M = Rx(a);
                break;
            }
            case 4:{
                double a; cout<<"angle (deg): "; cin>>a;
                M = Ry(a);
                break;
            }
            case 5:{
                double a; cout<<"angle (deg): "; cin>>a;
                M = Rz(a);
                break;
            }
            case 6: M = ReflectXY(); break;
            case 7: M = ReflectYZ(); break;
            case 8: M = ReflectZX(); break;
            case 9: M = ReflectOrigin(); break;
            case 10:{
                double sh_xy,sh_xz,sh_yx,sh_yz,sh_zx,sh_zy;
                cout<<"Enter 6 shears (sh_xy sh_xz sh_yx sh_yz sh_zx sh_zy): ";
                cin>>sh_xy>>sh_xz>>sh_yx>>sh_yz>>sh_zx>>sh_zy;
                M = Shear(sh_xy,sh_xz,sh_yx,sh_yz,sh_zx,sh_zy);
                break;
            }
            default:
                cout<<"Invalid choice.\n";
                continue;
        }

        // Compose: NEW on the LEFT (since P' = M * P; last added executes last)
        M_total = mul(M, M_total);

        cout<<"\nCurrent transform matrix (M_total = latest * previous):\n";
        printMat(M_total);
    }

    auto out = transformAll(M_total, pts);
    printPts(out, "Transformed Points");

    cout<<"\nFinal 4x4 transform matrix used:\n";
    printMat(M_total);

    cout<<"\nNote: Order matters. Because we use column vectors, the transform entered last\n"
          "acts last (M_total = M_last * ... * M_first).\n";
    return 0;
}
