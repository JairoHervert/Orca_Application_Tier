-- MySQL dump 10.13  Distrib 8.4.6, for Linux (x86_64)
--
-- Host: localhost    Database: orca
-- ------------------------------------------------------
-- Server version	8.4.6

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `commits`
--

DROP TABLE IF EXISTS `commits`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `commits` (
  `idcommits` int unsigned NOT NULL AUTO_INCREMENT,
  `iduser` int unsigned NOT NULL,
  `idsourcefile` int unsigned DEFAULT NULL,
  `digitalsignature` text CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci,
  `isaccepted` tinyint unsigned NOT NULL,
  `date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `command` varchar(100) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `description` varchar(800) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT NULL,
  PRIMARY KEY (`idcommits`),
  KEY `fk_commits_users` (`iduser`),
  KEY `fk_commits_sourcefiles` (`idsourcefile`),
  CONSTRAINT `fk_commits_sourcefiles` FOREIGN KEY (`idsourcefile`) REFERENCES `sourcefiles` (`idsourcefile`),
  CONSTRAINT `fk_commits_users` FOREIGN KEY (`iduser`) REFERENCES `users` (`iduser`)
) ENGINE=InnoDB AUTO_INCREMENT=240 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `filepermissions`
--

DROP TABLE IF EXISTS `filepermissions`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `filepermissions` (
  `iduser` int unsigned NOT NULL,
  `idsourcefile` int unsigned NOT NULL,
  PRIMARY KEY (`iduser`,`idsourcefile`),
  UNIQUE KEY `idx_Permissions_UNIQUE` (`iduser`,`idsourcefile`),
  KEY `fk_filepermissions_sourcefiles` (`idsourcefile`),
  CONSTRAINT `fk_filepermissions_sourcefiles` FOREIGN KEY (`idsourcefile`) REFERENCES `sourcefiles` (`idsourcefile`),
  CONSTRAINT `fk_filepermissions_users` FOREIGN KEY (`iduser`) REFERENCES `users` (`iduser`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `projects`
--

DROP TABLE IF EXISTS `projects`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `projects` (
  `idproject` int unsigned NOT NULL AUTO_INCREMENT,
  `projectname` varchar(100) NOT NULL,
  `description` text NOT NULL,
  `idowner` int unsigned NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`idproject`),
  UNIQUE KEY `ProjectName_UNIQUE` (`projectname`),
  KEY `fk_projects_users1_idx` (`idowner`),
  CONSTRAINT `fk_projects_users1` FOREIGN KEY (`idowner`) REFERENCES `users` (`iduser`)
) ENGINE=InnoDB AUTO_INCREMENT=20 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `repo_protect`
--

DROP TABLE IF EXISTS `repo_protect`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `repo_protect` (
  `iduser` int unsigned NOT NULL,
  `idproject` int unsigned NOT NULL,
  `project_alias` varchar(100) NOT NULL,
  `rsa_aes` varchar(500) DEFAULT NULL,
  PRIMARY KEY (`iduser`,`idproject`,`project_alias`),
  KEY `fk_repo_protect_project` (`idproject`),
  CONSTRAINT `fk_repo_protect_project` FOREIGN KEY (`idproject`) REFERENCES `projects` (`idproject`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `fk_repo_protect_user` FOREIGN KEY (`iduser`) REFERENCES `users` (`iduser`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `sourcefiles`
--

DROP TABLE IF EXISTS `sourcefiles`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `sourcefiles` (
  `idsourcefile` int unsigned NOT NULL AUTO_INCREMENT,
  `idproject` int unsigned NOT NULL,
  `route` varchar(255) NOT NULL,
  PRIMARY KEY (`idsourcefile`),
  UNIQUE KEY `idx_project_route` (`idproject`,`route`),
  KEY `fk_SourceFile_Projects_idx` (`idproject`) /*!80000 INVISIBLE */,
  CONSTRAINT `fk_sourcefile_projects` FOREIGN KEY (`idproject`) REFERENCES `projects` (`idproject`) ON DELETE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `users`
--

DROP TABLE IF EXISTS `users`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `users` (
  `iduser` int unsigned NOT NULL AUTO_INCREMENT,
  `email` varchar(50) NOT NULL,
  `name` varchar(100) NOT NULL,
  `password` varchar(256) DEFAULT NULL,
  `role` int unsigned NOT NULL DEFAULT '1',
  `status` tinyint NOT NULL DEFAULT '1',
  `kpubecdsa` varchar(500) DEFAULT NULL,
  `verify` tinyint NOT NULL DEFAULT '0',
  `kpubrsa` varchar(500) DEFAULT NULL,
  PRIMARY KEY (`iduser`),
  UNIQUE KEY `Email_UNIQUE` (`email`) /*!80000 INVISIBLE */
) ENGINE=InnoDB AUTO_INCREMENT=22 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `users_has_projects`
--

DROP TABLE IF EXISTS `users_has_projects`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `users_has_projects` (
  `iduser` int unsigned NOT NULL,
  `idproject` int unsigned NOT NULL,
  PRIMARY KEY (`iduser`,`idproject`),
  KEY `fk_users_has_projects_projects1_idx` (`idproject`),
  KEY `fk_users_has_projects_users1_idx` (`iduser`),
  CONSTRAINT `fk_users_has_projects_projects1` FOREIGN KEY (`idproject`) REFERENCES `projects` (`idproject`),
  CONSTRAINT `fk_users_has_projects_users1` FOREIGN KEY (`iduser`) REFERENCES `users` (`iduser`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping routines for database 'orca'
--
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2025-12-14 12:09:39
