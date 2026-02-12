#!/bin/bash

# ==============================================================================
# SCRIPT DE GESTION (bash.sh) - AVEC AUTO-INSTALL & MAKEFILE
# ==============================================================================

set -e  # Arrêter en cas d'erreur critique

# --- Variables ---
PROGRAM_SOURCE="programme.c"
PROGRAM_NAME="programme" 
INSTALLED_BIN_NAME="mon_processus_resident"
SERVICE_NAME="mon_processus_resident"
LOG_FILE="/tmp/processus_resident.log"
LOG_TEMP="/tmp/c_test_output.log" 

# Chemins Kernel
KERNEL_MODULE_SOURCE="hide_process.c"
KERNEL_MODULE_NAME="hide_process"
KERNEL_MODULE_FILE="${KERNEL_MODULE_NAME}.ko"
KERNEL_VERSION=$(uname -r)
MODULE_DEST_DIR="/lib/modules/$KERNEL_VERSION/extra"

# Chemins Système
BIN_DEST_DIR="/usr/local/bin"

# --- Couleurs ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# --- Fonctions d'affichage ---
print_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[✓]${NC} $1"; }
print_error() { echo -e "${RED}[✗]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[!]${NC} $1"; }

# --- Vérifications ---

check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "Ce script doit être exécuté avec sudo (./deploy_manager.sh)"
        exit 1
    fi
}

# Fonction améliorée pour gérer les dépendances sans forcer APT
check_dependencies() {
    print_info "Vérification de l'environnement de compilation..."

    if [ -d "/lib/modules/$(uname -r)/build" ]; then
        print_success "Headers du noyau détectés localement."
    else
        print_warning "Headers introuvables. Tentative d'installation automatique..."
        
        if command -v apt-get >/dev/null; then
            apt-get update
            apt-get install -y build-essential "linux-headers-$(uname -r)" kmod || \
            apt-get install -y build-essential linux-headers-amd64 kmod
        elif command -v pacman >/dev/null; then
            pacman -S --noconfirm linux-headers base-devel kmod
        elif command -v dnf >/dev/null; then
            dnf install -y kernel-devel-"$(uname -r)" kmod
        elif command -v yum >/dev/null; then
            yum install -y kernel-devel-"$(uname -r)" kmod
        else
            print_error "Impossible de trouver un gestionnaire de paquets supporté."
            exit 1
        fi
    fi
    
    if [ ! -d "/lib/modules/$(uname -r)/build" ]; then
        print_error "ÉCHEC : Les headers du noyau semblent toujours manquants."
        echo "Essayez : sudo apt install -y linux-headers-\$(uname -r)"
        exit 1
    fi
    print_success "Environnement prêt."
}

generate_makefile() {
    if [ ! -f "Makefile" ]; then
        print_info "Génération du Makefile..."
        cat <<EOF > Makefile
obj-m += $KERNEL_MODULE_NAME.o

all:
	make -C /lib/modules/\$(shell uname -r)/build M=\$(PWD) modules

clean:
	make -C /lib/modules/\$(shell uname -r)/build M=\$(PWD) clean
EOF
        print_success "Makefile créé."
    fi
}

compile_program() {
    print_info "Compilation du programme C ($PROGRAM_SOURCE)..."
    if [ ! -f "$PROGRAM_SOURCE" ]; then
        print_error "Fichier $PROGRAM_SOURCE introuvable!"
        exit 1
    fi
    
    gcc -o $PROGRAM_NAME $PROGRAM_SOURCE
    chmod +x $PROGRAM_NAME
    print_success "Compilation C réussie: $PROGRAM_NAME"
}

compile_kernel_module() {
    print_info "Compilation du module kernel..."
    if [ ! -f "$KERNEL_MODULE_SOURCE" ]; then
        print_error "Fichier source $KERNEL_MODULE_SOURCE introuvable!"
        exit 1
    fi
    
    generate_makefile
    
    make clean >/dev/null 2>&1
    make >/dev/null
    
    if [ -f "$KERNEL_MODULE_FILE" ]; then
        print_success "Compilation Kernel réussie: $KERNEL_MODULE_FILE"
    else
        print_error "Échec compilation module (Vérifiez les erreurs GCC ci-dessus)"
        make
        exit 1
    fi
}

load_kernel_module_temp() {
    rmmod $KERNEL_MODULE_NAME 2>/dev/null || true
    insmod $KERNEL_MODULE_FILE
    
    if lsmod | grep -q "$KERNEL_MODULE_NAME"; then
        print_success "Module kernel chargé temporairement"
    else
        print_error "Erreur lors du chargement (insmod)."
        exit 1
    fi
}


# --- FONCTION PCB MONITORING (POLICY REMOVED) ---
display_process_pcb() {
    local pid=$1
    if [ -z "$pid" ] || [ "$pid" -eq 0 ]; then
        print_error "PID invalide ou processus non démarré."
        return 1
    fi

    local status_file="/proc/$pid/status"
    
    if [ ! -f "$status_file" ]; then
        print_warning "Impossible de lire /proc/$pid/status (Processus caché ou terminé ?)"
        return 1
    fi

    # --- 1. Extraction Identifiants ---
    local pid_val=$(grep '^Pid:' "$status_file" | awk '{print $2}')
    local ppid_val=$(grep '^PPid:' "$status_file" | awk '{print $2}')
    local uid_val=$(grep '^Uid:' "$status_file" | awk '{print $2}')
    local gid_val=$(grep '^Gid:' "$status_file" | awk '{print $2}')
    
    # --- 2. Extraction Priorité (Dynamique) ---
    local nice_val="Inconnu"
    local real_prio="Inconnu"

    # Méthode A: Via ps (le plus lisible)
    if command -v ps >/dev/null; then
        # pri = Priorité Kernel mappée
        # ni  = Nice value
        read real_prio nice_val <<< $(ps -p "$pid" -o pri=,ni= --no-headers 2>/dev/null)
    fi

    # Méthode B: Fallback raw via /proc/pid/stat (Si ps échoue)
    if [ -z "$real_prio" ] || [ "$real_prio" == "Inconnu" ]; then
         if [ -f "/proc/$pid/stat" ]; then
             # Champ 18: Priority, Champ 19: Nice
             real_prio=$(awk '{print $18}' "/proc/$pid/stat")
             nice_val=$(awk '{print $19}' "/proc/$pid/stat")
         fi
    fi

    # NOTE: La section "Policy" a été supprimée ici.

    echo -e "\n${YELLOW}======================================================${NC}"
    echo -e "${YELLOW}    PROCESS CONTROL BLOCK (PCB) - PID: $pid_val    ${NC}"
    echo -e "${YELLOW}======================================================${NC}"
    
    echo -e "    ${BLUE}1. IDENTIFICATION${NC}"
    echo -e "    -----------------"
    echo -e "    PID       : ${RED}$pid_val${NC}"
    echo -e "    PPID      : ${RED}$ppid_val${NC}"
    echo -e "    User ID   : ${RED}$uid_val${NC} (root=0)"
    echo -e "    Group ID  : ${RED}$gid_val${NC} (root=0)"
    
    echo -e "\n    ${BLUE}2. ORDONNANCEMENT (PRIORITÉ)${NC}"
    echo -e "    --------------------------"
    echo -e "    Real Priority : ${RED}$real_prio${NC} (Dynamic Scheduler Priority)"
    echo -e "    Nice Value    : ${RED}$nice_val${NC} (Static Hint)"
    echo -e "${YELLOW}======================================================${NC}"

    return 0
}

# --- Option 1: Installation Complète ---
setup_persistence() {
    print_info "Installation persistante..."
    
    mkdir -p "$MODULE_DEST_DIR"
    cp "$KERNEL_MODULE_FILE" "$MODULE_DEST_DIR/"
    depmod -a
    if ! grep -q "^$KERNEL_MODULE_NAME" /etc/modules; then
        echo "$KERNEL_MODULE_NAME" >> /etc/modules
    fi

    cp "$PROGRAM_NAME" "$BIN_DEST_DIR/$INSTALLED_BIN_NAME"
    chmod +x "$BIN_DEST_DIR/$INSTALLED_BIN_NAME"
}

full_install_routine() {
    check_root
    check_dependencies
    echo ""
    compile_program
    compile_kernel_module
    echo ""
    
    load_kernel_module_temp 
    setup_persistence
    
    print_info "Création et démarrage du service..."
    
    ./$PROGRAM_NAME --install >/dev/null
    
    local service_file="/etc/systemd/system/${SERVICE_NAME}.service"
    if [ -f "$service_file" ]; then
        sed -i "s|ExecStart=.*|ExecStart=$BIN_DEST_DIR/$INSTALLED_BIN_NAME --daemon|g" "$service_file"
    fi

    systemctl daemon-reload
    systemctl enable $SERVICE_NAME
    systemctl start $SERVICE_NAME
    
    print_success "Service démarré."

    sleep 1
    local target_pid=$(systemctl show --property MainPID --value $SERVICE_NAME)
    
    if [ "$target_pid" != 0 ] && [ ! -z "$target_pid" ]; then
        print_info "Affichage PCB du Daemon (Service Systemd) :"
        display_process_pcb "$target_pid"
    else
        print_warning "PID introuvable via systemd."
    fi
    
    echo ""
    print_success "Installation terminée. Le processus tourne en arrière-plan."
}

# --- Option 2: Test Interactif ---
test_routine() {
    check_root
    compile_program
    compile_kernel_module
    load_kernel_module_temp
    
    rm -f "$LOG_TEMP"
    
    print_info "Lancement du programme C (PID à cacher)..."
    ./$PROGRAM_NAME > "$LOG_TEMP" 2>&1 &
    local c_parent_pid=$!

    sleep 2 

    echo -e "\n${BLUE}--- SORTIE PROGRAMME C ---${NC}"
    cat "$LOG_TEMP"
    echo -e "${BLUE}--------------------------${NC}\n"

    local target_pid=$(grep 'PID DU PROCESSUS FILS (ENFANT)' "$LOG_TEMP" | awk '{print $NF}' | tail -n 1)
    
    if [ ! -z "$target_pid" ] && [ "$target_pid" != 0 ]; then
        print_success "Cible détectée : PID $target_pid"
        
        display_process_pcb "$target_pid"
        
        print_warning "Le processus est maintenant caché. Vérifiez 'ps aux' ou 'top'."
        read -p "Appuyez sur [Entrée] pour nettoyer et quitter..."

        kill -9 $target_pid 2>/dev/null || true
    else
        print_error "Impossible de trouver le PID dans la sortie."
    fi
    
    rmmod $KERNEL_MODULE_NAME 2>/dev/null || true
    rm -f "$LOG_TEMP"
    print_success "Nettoyage effectué."
}

# --- Menu et Désinstallation ---
uninstall_full() {
    systemctl stop $SERVICE_NAME 2>/dev/null || true
    systemctl disable $SERVICE_NAME 2>/dev/null || true
    rm -f "/etc/systemd/system/${SERVICE_NAME}.service"
    systemctl daemon-reload
    rm -f "$BIN_DEST_DIR/$INSTALLED_BIN_NAME"
    rmmod $KERNEL_MODULE_NAME 2>/dev/null || true
    rm -f "$MODULE_DEST_DIR/$KERNEL_MODULE_FILE"
    depmod -a
    sed -i "/^$KERNEL_MODULE_NAME/d" /etc/modules
    make clean 2>/dev/null || true
    rm -f $PROGRAM_NAME
    rm -f "Makefile"
    rm -f "hide_process.mod.c" "hide_process.mod" "hide_process.o" "modules.order" "Module.symvers"
    print_success "Tout a été désinstallé."
}

show_menu() {
    clear
    echo "╔════════════════════════════════════════════════╗"
    echo "║    MANAGER PROCESSUS (deploy_manager.sh)       ║"
    echo "╚════════════════════════════════════════════════╝"
    echo "1) Installation Complète (Affiche PCB)"
    echo "2) Test Interactif (Affiche PCB)"
    echo "3) Vérifier statut"
    echo "4) Logs"
    echo "5) Désinstaller tout"
    echo "6) Quitter"
    echo ""
    read -p "Choix: " choice

    case $choice in
        1) full_install_routine ;;
        2) test_routine ;;
        3) systemctl status $SERVICE_NAME --no-pager ;;
        4) tail -n 20 $LOG_FILE ;;
        5) uninstall_full ;;
        6) exit 0 ;;
        *) print_error "Choix invalide" ;;
    esac
}

# --- Main ---
if [ $# -gt 0 ]; then
    case $1 in
        --install) full_install_routine ;;
        --uninstall) check_root; uninstall_full ;;
        --test) test_routine ;;
        *) echo "Usage: $0 [--install|--uninstall|--test]" ;;
    esac
else
    show_menu
fi
