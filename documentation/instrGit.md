# 📘 Guía de Git para Estudiantes

---


```bash

# 1. Actualiza tu copia local de main antes de empezar
git checkout main
# Cambia a la rama principal para partir de la versión más reciente

git pull origin main
# Descarga los últimos cambios del repositorio remoto

# 2. Verifica en qué rama estás
git status
# Debe mostrar "On branch main" — si no, cámbiate con "git checkout main"

# 3. Crea tu rama de trabajo (usa SIEMPRE el mismo nombre en los pasos siguientes)
git checkout -b apellido-tarea1
# Crea una rama nueva y te cambia a ella automáticamente
# Si la rama ya existe (por ejemplo, sesión anterior), usa en su lugar:
# git checkout apellido-tarea1

# --- Aquí realizas y guardas tus ejercicios ---

# 4. Agrega los archivos modificados al área de staging
git add .
# Prepara todos los cambios (nuevos y modificados) para el commit

# 5. Guarda los cambios localmente con un mensaje descriptivo
git commit -m "Tarea 1 completada - Tu Nombre"
# Crea un punto de guardado en el historial con ese mensaje

# 6. Sube tu rama a GitHub
git push -u origin apellido-tarea1
# Sube la rama al remoto y la vincula como upstream
# (después de esto, solo necesitas "git push" para futuras subidas a esta misma rama)

# 7. Entrega final: abre un Pull Request en GitHub
# Ve al repositorio en el navegador → "Compare & pull request" → describe tu trabajo → Create pull request

---

## 🔄 2. Ciclo para Iniciar una Nueva Tarea

Una vez que el profesor haya aprobado y unido (Merged) las tareas en GitHub, sigue esta rutina para limpiar tu computadora y empezar el siguiente ejercicio:

```bash
# 0. Verifica que no tengas cambios sin guardar en tu rama actual
git status
# Si aparece algo sin comitear, guárdalo o descártalo antes de continuar

# 1. Regresa a la rama principal
git checkout main
# Cambia de tu rama de tarea de vuelta a main

# 2. Descarga las tareas aprobadas de todos tus compañeros
git pull origin main
# Trae los últimos cambios fusionados (merged) desde GitHub

# 3. Borra tu rama de la tarea anterior (ya está a salvo en main)
git branch -d apellido-tarea1
# Elimina la rama local; si el PR se fusionó con "Squash and merge",
# este comando puede fallar con "not fully merged" — en ese caso,
# usa "git branch -D apellido-tarea1" (mayúscula) para forzar el borrado

# 4. Crea y cámbiate a la rama de la nueva tarea
git checkout -b apellido-tarea2
# Crea una rama nueva a partir de la main actualizadagit 
```

---

## 🚨 En Caso de Emergencia (Errores Comunes)



### ❌ Error: "error: The branch... is not fully merged"
* **Por qué pasa:** Intentaste borrar tu rama vieja, pero el profesor aún no la aprueba en GitHub o no has hecho `git pull`.
* **Solución:** Si estás seguro de querer borrarla de todos modos, usa la fuerza con la **D mayúscula**:
  ```bash
  git branch -D tu-nombre-tarea1
  ```
