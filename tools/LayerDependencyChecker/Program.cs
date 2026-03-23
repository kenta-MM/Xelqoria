using System.Text.RegularExpressions;

var repoDir = GetRepoDir(args);

var allowedDependencies = new Dictionary<string, HashSet<string>>(StringComparer.OrdinalIgnoreCase)
{
    ["App"] = new(StringComparer.OrdinalIgnoreCase) { "App", "Backends", "Core", "Game", "Graphics", "RHI" },
    ["Backends"] = new(StringComparer.OrdinalIgnoreCase) { "Backends", "RHI" },
    ["Core"] = new(StringComparer.OrdinalIgnoreCase) { "Core" },
    ["Editor"] = new(StringComparer.OrdinalIgnoreCase) { "Backends", "Core", "Editor", "Game", "Graphics", "RHI" },
    ["Game"] = new(StringComparer.OrdinalIgnoreCase) { "Core", "Game", "Graphics" },
    ["Graphics"] = new(StringComparer.OrdinalIgnoreCase) { "Core", "Graphics", "RHI" },
    ["RHI"] = new(StringComparer.OrdinalIgnoreCase) { "RHI" },
};

var projectDirs = new[]
{
    "RHI",
    "Graphics",
    "Backends.D3D11",
    "Backends.D3D12",
    "Core",
    "App",
    "Editor",
    "Game",
};

var sourceRoots = new List<string>();
var sourceRootLayers = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

foreach (var projectName in projectDirs)
{
    var sourceRoot = Path.Combine(repoDir, projectName, "Source");
    if (!Directory.Exists(sourceRoot))
    {
        continue;
    }

    var normalizedSourceRoot = Path.GetFullPath(sourceRoot);
    sourceRoots.Add(normalizedSourceRoot);
    sourceRootLayers[normalizedSourceRoot] = projectName.StartsWith("Backends", StringComparison.OrdinalIgnoreCase)
        ? "Backends"
        : projectName;
}

var sourceFiles = sourceRoots
    .SelectMany(root => Directory.EnumerateFiles(root, "*.h", SearchOption.AllDirectories)
        .Concat(Directory.EnumerateFiles(root, "*.cpp", SearchOption.AllDirectories)));

var includePattern = new Regex("^\\s*#include\\s+\"([^\"]+)\"", RegexOptions.Compiled);
var violations = new List<string>();

foreach (var file in sourceFiles)
{
    var sourceLayer = GetLayerName(file, sourceRoots, sourceRootLayers);
    if (sourceLayer is null || !allowedDependencies.TryGetValue(sourceLayer, out var allowedTargets))
    {
        continue;
    }

    var lineNumber = 0;
    foreach (var line in File.ReadLines(file))
    {
        lineNumber += 1;
        var match = includePattern.Match(line);
        if (!match.Success)
        {
            continue;
        }

        var includePath = match.Groups[1].Value;
        var resolvedInclude = ResolveIncludePath(file, includePath, sourceRoots);
        var targetLayer = GetLayerName(resolvedInclude, sourceRoots, sourceRootLayers);

        if (targetLayer is null)
        {
            continue;
        }

        if (!allowedTargets.Contains(targetLayer))
        {
            var relativeSource = Path.GetRelativePath(repoDir, file);
            var relativeTarget = Path.GetRelativePath(repoDir, resolvedInclude);
            violations.Add(
                $"{relativeSource}:{lineNumber}: {sourceLayer} cannot depend on {targetLayer}. " +
                $"include=\"{includePath}\", resolved=\"{relativeTarget}\"");
        }
    }
}

if (violations.Count > 0)
{
    Console.ForegroundColor = ConsoleColor.Red;
    Console.Error.WriteLine("[Error] Layer dependency violations detected:");
    foreach (var violation in violations)
    {
        Console.Error.WriteLine($"[Error] {violation}");
    }

    Console.ResetColor();
    return 1;
}

Console.WriteLine("Layer dependency check passed.");
return 0;

static string GetRepoDir(string[] args)
{
    if (args.Length != 1 || string.IsNullOrWhiteSpace(args[0]))
    {
        Console.Error.WriteLine("Usage: LayerDependencyChecker <RepoDir>");
        Environment.Exit(1);
    }

    return Path.GetFullPath(args[0].Trim('"'));
}

static string? GetLayerName(string filePath, IReadOnlyList<string> sourceRoots, IReadOnlyDictionary<string, string> sourceRootLayers)
{
    var normalizedPath = Path.GetFullPath(filePath);

    foreach (var sourceRoot in sourceRoots)
    {
        if (!normalizedPath.StartsWith(EnsureTrailingSeparator(sourceRoot), StringComparison.OrdinalIgnoreCase) &&
            !string.Equals(normalizedPath, sourceRoot, StringComparison.OrdinalIgnoreCase))
        {
            continue;
        }

        return sourceRootLayers[sourceRoot];
    }

    return null;
}

static string ResolveIncludePath(string sourceFile, string includePath, IReadOnlyList<string> sourceRoots)
{
    var sourceDirectory = Path.GetDirectoryName(sourceFile) ?? string.Empty;
    var localCandidate = Path.GetFullPath(Path.Combine(sourceDirectory, includePath));
    if (File.Exists(localCandidate))
    {
        return localCandidate;
    }

    foreach (var root in sourceRoots)
    {
        var candidate = Path.Combine(root, includePath);
        if (File.Exists(candidate))
        {
            return candidate;
        }
    }

    return localCandidate;
}

static string EnsureTrailingSeparator(string path)
{
    return path.EndsWith(Path.DirectorySeparatorChar) || path.EndsWith(Path.AltDirectorySeparatorChar)
        ? path
        : path + Path.DirectorySeparatorChar;
}
