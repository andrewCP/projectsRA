# 📘 Guía de Git para Estudiantes

¡Hola de nuevo! Sigue estos pasos exactos para entregar tus tareas y comenzar las nuevas sin errores.

---

## 🚀 1. Ciclo para Entregar la Tarea Actual

Cuando termines de programar tus ejercicios, ejecuta estos comandos en tu terminal en este orden:

```bash
# 1. Asegúrate de estar en tu rama de trabajo (no en main)
git status

# 2. Prepara todos tus archivos modificados
git add .

# 3. Guarda tus cambios localmente con un mensaje descriptivo
git commit -m "Tarea 1 completada - Tu Nombre"

# 4. Sube tu rama a GitHub
git push -u origin tu-nombre-tarea1
```

> 🌐 **Paso final en la Web:** Entra al enlace de GitHub del proyecto, haz clic en el botón verde **"Compare & pull request"**, escribe un mensaje breve y envíalo. ¡El profesor revisará tu código!

---

## 🔄 2. Ciclo para Iniciar una Nueva Tarea

Una vez que el profesor haya aprobado y unido (Merged) las tareas en GitHub, sigue esta rutina para limpiar tu computadora y empezar el siguiente ejercicio:

```bash
# 1. Regresa a la rama principal
git checkout main

# 2. Descarga las tareas aprobadas de todos tus compañeros
git pull

# 3. Borra tu rama de la tarea anterior (ya está a salvo en main)
git branch -d tu-nombre-tarea1

# 4. Crea y cámbiate a la rama de la nueva tarea
git checkout -b tu-nombre-tarea2
```

---

## 🚨 En Caso de Emergencia (Errores Comunes)

### ❌ Error: "Protected branch..." al hacer git push
* **Por qué pasa:** Olvidaste crear tu rama y trabajaste directamente en `main`. Como `main` está protegida, GitHub bloquea tu subida.
* **Solución (Ejecuta estos 3 comandos):**
  ```bash
  git branch tu-nombre-tarea1          # Guarda tus cambios en una rama nueva
  git reset --hard origin/main         # Limpia tu rama main local
  git checkout tu-nombre-tarea1        # Cámbiate a tu rama correcta
  ```
  *(Ahora ya puedes hacer el `git push -u origin tu-nombre-tarea1` con éxito)*

### ❌ Error: "error: The branch... is not fully merged"
* **Por qué pasa:** Intentaste borrar tu rama vieja, pero el profesor aún no la aprueba en GitHub o no has hecho `git pull`.
* **Solución:** Si estás seguro de querer borrarla de todos modos, usa la fuerza con la **D mayúscula**:
  ```bash
  git branch -D tu-nombre-tarea1
  ```
