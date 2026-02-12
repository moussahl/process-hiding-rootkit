#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define SERVICE_NAME "mon_processus_resident"
#define LOG_FILE "/tmp/processus_resident.log"

// Fonction pour logger les messages
void log_message(const char *msg)
{
	FILE *f = fopen(LOG_FILE, "a");
	if (f)
	{
		fprintf(f, "[PID %d] %s\n", getpid(), msg);
		fclose(f);
	}
}

// Fonction pour créer le service systemd
int create_systemd_service()
{
	char service_path[256];
	char exec_path[256];

	// Obtenir le chemin complet de l'exécutable
	if (readlink("/proc/self/exe", exec_path, sizeof(exec_path) - 1) == -1)
	{
		perror("Erreur lors de la lecture du chemin de l'exécutable");
		return -1;
	}

	snprintf(service_path, sizeof(service_path),
			 "/etc/systemd/system/%s.service", SERVICE_NAME);

	FILE *f = fopen(service_path, "w");
	if (!f)
	{
		perror("Erreur: Impossible de créer le fichier service (exécutez avec sudo)");
		return -1;
	}

	fprintf(f, "[Unit]\n");
	fprintf(f, "Description=Processus Résident avec Priorité Modifiée\n");
	fprintf(f, "After=network.target\n\n");
	fprintf(f, "[Service]\n");
	fprintf(f, "Type=simple\n");
	// IMPORTANT: On passe --daemon pour que le service lance la bonne boucle
	fprintf(f, "ExecStart=%s --daemon\n", exec_path);
	fprintf(f, "Restart=always\n");
	fprintf(f, "RestartSec=10\n");
	fprintf(f, "Nice=-10\n"); // Priorité élevée gérée par systemd
	fprintf(f, "CPUSchedulingPolicy=other\n\n");
	fprintf(f, "[Install]\n");
	fprintf(f, "WantedBy=multi-user.target\n");
	fclose(f);

	printf("✓ Service créé: %s\n", service_path);
	printf("\nPour activer le service au démarrage:\n");
	printf("   sudo systemctl daemon-reload\n");
	printf("   sudo systemctl enable %s\n", SERVICE_NAME);
	printf("   sudo systemctl start %s\n", SERVICE_NAME);
	printf("\nPour vérifier le statut:\n");
	printf("   sudo systemctl status %s\n", SERVICE_NAME);
	printf("   tail -f %s\n", LOG_FILE);

	return 0;
}

// Fonction pour modifier la priorité du processus
int modifier_priorite(pid_t pid, int nouvelle_priorite)
{
	if (setpriority(PRIO_PROCESS, pid, nouvelle_priorite) == -1)
	{
		perror("Erreur lors de la modification de la priorité");
		return -1;
	}
	int priorite = getpriority(PRIO_PROCESS, pid);
	printf("✓ Priorité du processus %d modifiée à: %d\n", pid, priorite);
	char log_msg[128];
	snprintf(log_msg, sizeof(log_msg), "Priorité modifiée à %d (nice value)", priorite);
	log_message(log_msg);
	return 0;
}

// Fonction daemon - le processus qui tourne en arrière-plan
void daemon_process()
{
	log_message("Démarrage du daemon");

	// Tentative de se cacher dès le démarrage du daemon
	// Cela nécessite que le module kernel soit déjà chargé
	kill(getpid(), 64);

	modifier_priorite(getpid(), -5);

	int count = 0;
	while (1)
	{
		char log_msg[128];
		snprintf(log_msg, sizeof(log_msg),
				 "Daemon actif - itération %d (priorité: %d)",
				 count++, getpriority(PRIO_PROCESS, getpid()));
		log_message(log_msg);
		sleep(30); // Attendre 30 secondes entre chaque log
	}
}

void afficher_aide()
{
	printf("Usage:\n");
	printf("   ./programme              - Créer et gérer un processus avec modification PCB\n");
	printf("   ./programme --install    - Installer comme service résident (nécessite sudo)\n");
	printf("   ./programme --daemon     - Mode daemon (utilisé par systemd)\n");
	printf("   ./programme --help       - Afficher cette aide\n");
}

// Fonction principale pour créer et gérer un processus enfant (Mode Test Interactif)
void creer_et_gerer_processus()
{
	pid_t pid;

	printf("=== Création d'un Processus avec Modification du PCB ===\n\n");
	printf("Processus parent PID: %d\n", getpid());

	pid = fork();

	if (pid < 0)
	{
		perror("Erreur lors de la création du processus");
		exit(1);
	}

	if (pid == 0)
	{
		// Code du processus enfant
		printf("→ Processus enfant créé! PID: %d\n", getpid());
		modifier_priorite(getpid(), -10);

		printf("\n→ Processus enfant en exécution (boucle infinie)...\n");
		printf("   Vous pouvez maintenant vérifier qu'il est caché (ex: ps aux)\n");

		while (1)
		{
			sleep(10);
		}
		exit(0);
	}

	else
	{
		// Code du processus parent
        // AFFICHAGE CLAIR DU PID DU FILS AVANT TENTATIVE DE CACHE
		printf("\n==================================================\n");
		printf("   PID DU PROCESSUS FILS (ENFANT) : %d\n", pid);
		printf("==================================================\n");

		sleep(1);

		printf("→ Parent: Envoi du signal 64 pour cacher le PID %d\n", pid);

		if (kill(pid, 64) == -1)
		{
			perror("Erreur lors de l'envoi du signal 64");
			printf("   Assurez-vous que le module 'hide_process.ko' est chargé!\n");
		}
		else
		{
			printf("✓ Signal 64 envoyé. Le processus %d devrait être caché.\n", pid);
		}

		printf("\n→ Processus parent attend la fin de l'enfant...\n");
		int status;
		waitpid(pid, &status, 0);
	}
}

int main(int argc, char *argv[])
{
	if (argc > 1)
	{
		if (strcmp(argv[1], "--install") == 0)
		{
			if (getuid() != 0)
			{
				fprintf(stderr, "Erreur: L'installation nécessite root (sudo)\n");
				return 1;
			}
			return create_systemd_service();
		}
		else if (strcmp(argv[1], "--daemon") == 0)
		{
			daemon_process();
			return 0;
		}
		else if (strcmp(argv[1], "--help") == 0)
		{
			afficher_aide();
			return 0;
		}
	}

	creer_et_gerer_processus();

	return 0;
}
