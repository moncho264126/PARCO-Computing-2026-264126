import os
import numpy as np
import re

# Directorio relativo donde están los .txt
RESULTS_DIR = "../results"

# Patrón para: weak_2d_np_1.txt, weak_2d_np_2.txt, etc.
FILE_PATTERN = re.compile(r"weak_2d_np_(\d+)\.txt")

def extract_times(filepath):
    """
    Lee un archivo de resultados y devuelve listas de tiempos
    (total, comm, comp).
    """
    total, comm, comp = [], [], []
    
    try:
        with open(filepath, "r") as f:
            for line in f:
                # Ignoramos líneas que no tengan el separador "|"
                # Esto filtra automáticamente la línea "Weak 2D scaling NP=X"
                if "|" not in line:
                    continue
                
                # Ignoramos la cabecera de columnas
                if "Total_Time" in line:
                    continue

                parts = [p.strip() for p in line.split("|")]
                
                # Formato esperado: Run | Total | Comm | Comp
                if len(parts) != 4:
                    continue
                
                try:
                    # Parseamos floats. Si falla (ej. "FAIL"), salta al except
                    t_tot = float(parts[1])
                    t_comm = float(parts[2])
                    t_comp = float(parts[3])
                    
                    total.append(t_tot)
                    comm.append(t_comm)
                    comp.append(t_comp)
                except ValueError:
                    continue
    except FileNotFoundError:
        print(f"Error: File {filepath} not found.")
        return [], [], []
                
    return np.array(total), np.array(comm), np.array(comp)

def p90(arr):
    """Calcula el percentil 90 para estabilidad."""
    return np.percentile(arr, 90) if len(arr) > 0 else None

def parse_weak_2d():
    """
    Recorre el directorio RESULTS_DIR y extrae datos de todos
    los archivos que cumplan el patrón weak_2d.
    """
    results = {}
    
    if not os.path.exists(RESULTS_DIR):
        print(f"Warning: Directory {RESULTS_DIR} does not exist.")
        return {}

    for fname in os.listdir(RESULTS_DIR):
        m = FILE_PATTERN.match(fname)
        if not m:
            continue
        
        np_val = int(m.group(1))
        path = os.path.join(RESULTS_DIR, fname)
        
        total, comm, comp = extract_times(path)
        
        if len(total) == 0:
            print(f"Warning: No valid data in {fname}")
            continue
            
        results[np_val] = {
            "total": p90(total),
            "comm":  p90(comm),
            "comp":  p90(comp),
        }
        
    # Devolvemos diccionario ordenado por número de procesos
    return dict(sorted(results.items()))

if __name__ == "__main__":
    # Test rápido si se ejecuta directamente
    data = parse_weak_2d()
    print(f"Found data for NPs: {list(data.keys())}")