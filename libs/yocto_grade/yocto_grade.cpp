//
// Implementation for Yocto/Grade.
//

//
// LICENSE:
//
// Copyright (c) 2020 -- 2020 Fabio Pellacini
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "yocto_grade.h"

// -----------------------------------------------------------------------------
// COLOR GRADING FUNCTIONS
// -----------------------------------------------------------------------------
namespace yocto::grade {
    struct rect2i
    {
        vec2i p;
        int width, height;
    };
    
    struct triangle2i
    {
        //3 vertici
        vec2i v[3];
        
        // restituisce il punto centrale del triangolo
        vec2i mid() { return  (v[0] + v[1] + v[2])/3; }
        
        //Crea in 'r' il bounding box per QUESTO triangolo
        void bounding_rectangle(rect2i& r)
        {
            //Calcola punto in alto a sinistra per il rettangolo 'r'
            r.p = {min(v[0].x, min(v[1].x, v[2].x)), min(v[0].y, min(v[1].y, v[2].y))};
            
            //Calcola il punto in basso a destra per il rettangolo 'r'
            vec2i tmp_p = {max(v[0].x, max(v[1].x, v[2].x)), max(v[0].y, max(v[1].y, v[2].y))};
            
            //Calcola larghezza e altezza
            r.width = abs(tmp_p.x - r.p.x);
            r.height = abs(tmp_p.y - r.p.y);
        }
        
        //controlla se un punto è all'interno del triangolo ==> contain_point() e sign() sono presi da stackoverflow : https://stackoverflow.com/a/2049593
        //prima veniva utilizzata una formula matematica basata sulle 3 aree che si possono creare con il punto p. Ma era poco efficiente
        bool contain_point(const vec2i& p)
        {
            int d1, d2, d3;
            bool has_neg, has_pos;

            d1 = sign(p, v[0], v[1]);
            d2 = sign(p, v[1], v[2]);
            d3 = sign(p, v[2], v[0]);

            has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

            return !(has_neg && has_pos);
        }
        
    private:
        int sign (const vec2i& p1, const vec2i& p2, const vec2i& p3) { return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y); }
    };
    
    
    void color_tint(vec3f& color,const vec3f& tint) { color *= tint; }

    void vignette(vec3f& color, const vec2i& size, const vec2i& ij, const float& vignette)
    {
        if(vignette != 0.f)
        {
            float vr = 1 - vignette;
            float r = length((vec2f) (ij - size/2)) / length((vec2f)(size/2));
            color *= 1 - smoothstep(vr, 2*vr, r);
        }
    }
        
    void film_grain(vec3f& color, rng_state& rng, const float& grain) { if(grain != 0.f) color += (rand1f(rng) - 0.5) * grain; }
        
    void saturation(vec3f& color, const float& saturation)
    {
        float g = (color.x + color.y + color.z) / 3;
        color = g + (color - g) * (saturation * 2);
    }
    
    void contrast(vec3f& color, const float& contrast) { color = gain(color, 1 - contrast); }
    
    void mosaic(vec3f& color, const img::image<vec4f>& graded, const vec2i& ij, const int& mosaic)
    {
        if(mosaic != 0) color = xyz(graded[{ij.x - ij.x % mosaic, ij.y - ij.y % mosaic}]);
    }
        
    void grid(vec3f& color, const vec2i& ij, const int& grid) { if(grid != 0) color = (0 == ij.x % grid || 0 == ij.y % grid) ? 0.5 * color : color;}
    
    //Funzione creata per passare da un vec3f a un vec4f
    vec4f xyz_to_xyzw(const vec3f& xyz, const float& w) { return vec4f(xyz, w); }
    
    // Funzione per ottenere il valore di un pixel in bianco e nero
    // (era inizialmente utilizzata per migliorare il sobel edge, ora non lo uso più)
    void rgb_to_grayscale(vec4f& rgb_pixel)
    {
        vec3f c = xyz(rgb_pixel);
        float i = c.x * 0.299f + c.y * 0.587f + c.z * 0.114f;
        rgb_pixel = {i, i, i, rgb_pixel.w};
    }
    
    //controlla se una cordinata è all'interno di una griglia
    bool inside(const vec2i& size, const vec2i& ij) { return (ij.x < size.x && ij.x >= 0) && (ij.y < size.y &&  ij.y >= 0); }
    
    //Implementazione sobel edge
    void sobel_egde(const img::image<vec4f>& src_img, img::image<vec4f>& dst_img)
    {
        for(int y=0; y<src_img.size().y; y++)
            for(int x=0; x<src_img.size().x; x++)
            {
                vec2i ij = {x,y};
                
                //X-Direction
                static std::unordered_map<vec2i, int> x_dir = { {{-1,-1}, -1}, {{-1,0},-2}, {{-1,1},-1}, {{1,-1},1}, {{1,0},2}, {{1,1},1} };
                vec4f gx = vec4f(0);
                
                for(auto const& [offs,v] : x_dir)
                    if(inside(src_img.size(), ij+offs)) gx += src_img[ij+offs] * v;
                
                //Y-Direction
                static std::unordered_map<vec2i, int> y_dir = { {{-1,-1}, -1}, {{0,-1},-2}, {{1,-1},-1}, {{-1,1},1}, {{0,1},2}, {{1,1},1} };
                vec4f gy = vec4f(0);
                
                for(auto const& [offs, v] : y_dir)
                    if(inside(src_img.size(), ij+offs)) gy+= src_img[ij+offs] * v;
                
                dst_img[ij] = abs(gx) + abs(gy);
            }
    }
    
    
    //Funzione che restituisce la distanza tra due punti
    int dist(const vec2i& p1, const vec2i& p2) { return abs(p1.x - p2.x) + abs(p1.y - p2.y); }
    
    //Funzione utilizzata per disegnare una linea : codice preso da stackoverflow e riadattato
    void drawline(img::image<vec4f>& img, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
    {
        const vec4f DEBUG_COLOR = vec4f(0,0,0,0);
        int dy = y1 - y0;
        int dx = x1 - x0;
        int stepx, stepy;

        if (dy < 0)
        {
            dy = -dy;
            stepy = -1;
        }
        else
            stepy = 1;
        if (dx < 0)
        {
            dx = -dx;
            stepx = -1;
        }
        else
            stepx = 1;

        dy <<= 1;
        dx <<= 1;

        y0 *= 1;
        y1 *= 1;
        img[{(int)x0,(int)y0}] = DEBUG_COLOR;

        if (dx > dy)
        {
            int fraction = dy - (dx >> 1);

            while (x0 != x1)
            {
                if (fraction >= 0)
                {
                    y0 += stepy;
                    fraction -= dx;
                }
                x0 += stepx;
                fraction += dy;
                img[{(int)x0,(int)y0}] = DEBUG_COLOR;
            }
        }
        else
        {
            int fraction = dx - (dy >> 1);

            while (y0 != y1)
            {
                if (fraction >= 0)
                {
                    x0 += stepx;
                    fraction -= dy;
                }
                y0 += stepy;
                fraction += dx;
                img[{(int)x0,(int)y0}] = DEBUG_COLOR;
            }
        }
    }
    
    //Funzione per disegnare i triangoli
    void draw_triangle(img::image<vec4f>& img, const triangle2i& t)
    {
        drawline(img, t.v[0].x, t.v[0].y, t.v[1].x, t.v[1].y);
        drawline(img, t.v[1].x, t.v[1].y, t.v[2].x, t.v[2].y);
        drawline(img, t.v[2].x, t.v[2].y, t.v[0].x, t.v[0].y);
    }
    
    //Funzione per selezionare i vertici che verrano utilizzati nella generazione dei triangoli
    void vertices_selection(const img::image<vec4f>& edge_mask, img::image<vec2i>& owner_grid, rng_state& rng, const float& edge_threshold, const float& edge_p, const float& not_edge_p)
    {
        vec2i edge_mask_size = edge_mask.size();
        owner_grid = img::image<vec2i>(edge_mask.size(), vec2i(-1,-1));
        
        //Selezione vertici --> Alta probabilità sulle cornici dell'immagine, Alta prob. sui bordi, bassa prob. sui pixel non-bordi
        
        //Sulle cornici verticali
        for(int y=0; y<edge_mask_size.y; y++)
        {
            for(int x : {0, edge_mask_size.x-1})
            {
                vec2i ij = {x,y};
                if(rand1f(rng)<0.4f) owner_grid[ij] = ij;
            }
        }
        
        //Sulle cornici orizzontali
        for(int x=0; x<edge_mask_size.x; x++)
            for(int y : {0, edge_mask_size.y-1})
            {
                vec2i ij = {x,y};
                if(rand1f(rng)<0.4f) owner_grid[{x,y}] = {x,y};
            }
        
        //In mezzo
        for(int y=1; y<edge_mask_size.y-1; y++)
        {
            for(int x=1; x<edge_mask_size.x-1; x++)
            {
                vec2i ij = {x,y};
                vec4f pxl = edge_mask[ij];
               
                if(pxl.x >= edge_threshold && pxl.y >= edge_threshold && pxl.z >= edge_threshold)
                {
                    if(rand1f(rng) < edge_p) owner_grid[ij] = ij;
                }
                else if(rand1f(rng) < not_edge_p) owner_grid[ij] = ij;

            }
        }
        
        //seleziona i pixel top-left, top-right, bottom-left, bottom-right come vertici
        owner_grid[{0,0}] = {0,0};
        owner_grid[{0, edge_mask_size.y-1}] = {0, edge_mask_size.y-1};
        owner_grid[{edge_mask_size.x-1, 0}] = {edge_mask_size.x-1, 0};
        owner_grid[edge_mask_size-1] = edge_mask_size-1;
    }
    
    //Funzione perr creare un voronoi graph (usato per cercare gli "owner" per la triangolarizzazione)
    void voronoi_graph(img::image<vec2i>& owner_grid)
    {
        vec2i owner_grid_size = owner_grid.size();
        int step = owner_grid_size.x > owner_grid_size.y ? owner_grid_size.x / 2 : owner_grid_size.y /2;
        while (step>=1)
        {
            //Inizializza o aggiorna gli offset
            vec2i offsets[] = { {step, 0}, {0, step}, {-step, step}, {step, -step}, {-step, -step}, {step, step}, {-step, 0}, {0, -step} };
            for(int y=0; y<owner_grid_size.y; y++)
                for(int x=0; x<owner_grid_size.x; x++)
                {
                    vec2i ij_pixelA = {x, y};
                    for(auto offs : offsets)
                    {
                        vec2i ij_pixelB = offs+ij_pixelA;
                        //Se il pixel in questione non ha un owner OR l'owner di pixelB è più vicino dell'onwer di pixelA ==> onwer di pixelA = owner di pixelB
                        if(inside(owner_grid_size, ij_pixelB) && (owner_grid[ij_pixelA] == vec2i(-1,-1) || dist(ij_pixelA, owner_grid[ij_pixelB]) <  dist(ij_pixelA, owner_grid[ij_pixelA]) ))
                            owner_grid[ij_pixelA] = owner_grid[ij_pixelB];
                        
                    }
                }
            //dimezza la distanza degli offset
            step /=  2;
        }
    }
    
    //Funzione usata per generare i triangoli utilizzando voronoi graph
    //Il parametro 'triangles' sarà la lista che conterrà i triangoli
    void generate_triangles(img::image<vec2i>& owner_grid, std::vector<triangle2i>& triangles)
    {
        //size della griglia
        vec2i owner_grid_size = owner_grid.size();
        
        //offset dei pixel 'vicini': pixel[x,y], pixel[x+1,y], pixel[x,y+1], pixel[x+1,y+1]
        vec2i neighbour_offs[] = { {0,0}, {1,0}, {0,1}, {1,1} };
        
        for(int y=0; y<owner_grid_size.y-1; y++)
        {
            for(int x=0; x<owner_grid_size.x-1; x++)
            {
                vec2i ij = {x,y};
                auto zones = std::unordered_set<vec2i>();
                
                //Prendi l' "owner" dei pixel 'vicini'
                for(const auto& offs : neighbour_offs) zones.insert(owner_grid[ij+offs]);
                
                //3 zone == 1 triangolo
                if(zones.size() == 3)
                {
                    triangle2i t;
                    int i=0;
                    for(vec2i coords : zones)
                        t.v[i++] = coords;
                    //aggiungi triangolo alla lista
                    triangles.push_back(t);
                }
                //4 zone == 2 triangoli --> primo triangolo: (p1,p2,p3) , secondo triangolo (p2,p3,p4)
                else if(zones.size() == 4)
                {
                    triangle2i t1, t2;
                    
                    //t1
                    t1.v[0] = owner_grid[ij];
                    t1.v[1] = owner_grid[ij+neighbour_offs[1]];
                    t1.v[2] = owner_grid[ij+neighbour_offs[2]];
                    
                    //t2
                    t2.v[0]  = owner_grid[ij+neighbour_offs[1]];
                    t2.v[1]  = owner_grid[ij+neighbour_offs[2]];
                    t2.v[2]  = owner_grid[ij+neighbour_offs[3]];
                    
                    //aggiungi i due triangoli alla lista
                    triangles.push_back(t1);
                    triangles.push_back(t2);
                }
            }
        }
    }
    
    //Funzione per renderizzare i triangoli
    void render_triangles(img::image<vec4f>& img, const std::vector<triangle2i>& triangles)
    {
        auto tmp = img::image<vec4f>(img.size());
        for(auto triangle : triangles)
        {
            //Calcola bounding box
            rect2i bounding_r;
            triangle.bounding_rectangle(bounding_r);
            
            for(int y=bounding_r.p.y; y<=bounding_r.p.y+bounding_r.height; y++)
            {
                for(int x=bounding_r.p.x; x<=bounding_r.p.x+bounding_r.width; x++)
                {
                    //Se p è nel triangolo allora disegnalo con il colore del centro
                    vec2i p = {x, y};
                    if(triangle.contain_point(p)) tmp[p] = img[triangle.mid()];
                }
            }
        }
        img = tmp;
    }

    /*
     Questa è la funzione da richiamare per la "creazione" del filtro extra creato per il primo homework.
     La funzione consiste in 5 fasi:
     
     1. Edge detection: in questa fase andiamo a "trovare" nell'immagine quelli che sono i bordi degli oggetti rappresentati,
     utilizzando un algoritmo chiamato "Sobel Edge"
     
     2. Dopo aver trovato i bordi, andiamo a selezionare su tutta l'immagine dei vertici che verranno utilizzati per la triangolarizzazione.
     Sui bordi appena trovati avremo una probabilità alta che un determinato pixel venga scelto come vertice.
     Sulle cornici avremo una probabilità ancora più alta.
     Sui punti "non-bordi" avremo una probabilità più bassa rispetto le due precedenti.
     
     3. Per ogni pixel[x,y] trovo tra i vertici selezionati nella fase precedente quello più vicino e mantengo questa informazione in owner.
     onwer[x,y] = cordinate del vertice più vicino
     
     4. In questa fase genero i triangoli.
     Per ogni pixel[x,y] vedo nei pixel: a destra, in basso e in basso a destra.
     Se i 4 pixel hanno 4 owner diversi --> avrò 2 triangoli
     Se i 4 pixel hanno 3 owner diversi --> avrò 1 triangolo
     
     5. In questa fase disegno i triangoli.
     Per ogni triangolo prendo il suo bounding box e disegno solo i pixel all'interno del triangolo, con il colore del pixel al centro del triangolo.
     
     Questo filtro è stato creato attraverso le informazioni trovate su questo sito: https://darkforte.github.io/LowPoly/ (Anche se è orientato ad utilizzare CUDA)
     
     Il codice è stato invece implementato da me.
     */
    void lowpolify(img::image<vec4f>& src_img, rng_state& rng, const grade_params& params)
    {
        if(!params.lowpoly) return;
        
        //prendi la grandezza dell'immagine
        vec2i src_image_size = src_img.size();
        
        //inizializza una "maschera" dove andremo ad evidenziare i bordi
        auto mask_edge = img::image<vec4f>(src_image_size);
        
        //1. Sobel Edge --> algoritmo  per "trovare" i bordi
        sobel_egde(src_img, mask_edge);
        
        //per ogni pixel manteniamo il pixel a cui "appartiene" nel voronoi graph
        auto owner = img::image<vec2i>(src_image_size);
        
        //2. Selezione dei vertici
        vertices_selection(mask_edge, owner, rng, params.edge_threshold, params.edge_p, params.not_edge_p);
        
        //3. Voronoi graph
        voronoi_graph(owner);
        auto triangles = std::vector<triangle2i>();
        
        //4. Genera triangoli
        generate_triangles(owner, triangles);
        
        //5. Renderizza i triangoli
        render_triangles(src_img, triangles);
        
        if(params.draw_triangles)
            for(auto& t : triangles) draw_triangle(src_img, t);
    }
    
    
        
    img::image<vec4f> grade_image(const img::image<vec4f>& img, const grade_params& params) {
        
        auto mask_edge = img::image<vec4f>(img.size());
        
        //applica tonemap
        auto graded = img::tonemap_image(img, params.exposure, params.filmic, params.srgb);
        //img::tonemap_image_mt(graded, img, params.exposure, params.filmic, params.srgb);
        
        rng_state rng = make_rng(time(nullptr));
        
        for (int y=0; y<img.size().y; y++)
        {
            for(int x=0; x<img.size().x; x++ )
            {
                //crea un vettore ij con le coordinate
                vec2i ij = {x, y};
                
                //prendi il "colore" del pixel
                vec3f color = xyz(graded[ij]);
                
                //fai il clamp dopo il tonemapping
                color = clamp(color, 0, 1);
                
                //color tint
                color_tint(color, params.tint);
                
                //saturation
                saturation(color, params.saturation);
                
                //contrast
                contrast(color, params.contrast);
                
                //vignette
                vignette(color, img.size(), ij, params.vignette);
                
                //film grain
                film_grain(color, rng, params.grain);
                
                //aggiorna il pixel
                graded[ij] = xyz_to_xyzw(color, img[ij].w);
                
                //mosaic
                mosaic(color, graded, ij, params.mosaic);
                
                //aggiorna di nuovo il pixel dopo mosaico..
                graded[ij] = xyz_to_xyzw(color, img[ij].w);
                
            }
            
        }
        
        //applica il filtro creato da me
        lowpolify(graded, rng, params);
        
        //grid
        for (int y=0; y<graded.size().y; y++)
        {
            for(int x=0; x<graded.size().x; x++)
            {
                //crea un vettore ij con le coordinate
                vec2i ij = {x, y};
                
                //ottieni il "colore" del pixel
                vec3f color = xyz(graded[ij]);
                
                //grid
                grid(color, ij, params.grid);
                
                //aggiorna il pixel di nuovo
                graded[ij] = xyz_to_xyzw(color, graded[ij].w);
                
            }
        }
        
        return graded;
    }

}  // namespace yocto::grade
